#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "blehostd.h"
#include "bhd_proto.h"
#include "bhd_util.h"
#include "syscfg/syscfg.h"
#include "sysinit/sysinit.h"
#include "os/os.h"
#include "native_sockets/native_sock.h"
#include "mcu/native_bsp.h"
#include "mn_socket/mn_socket.h"
#include "host/ble_hs.h"
#include "defs/error.h"

#define BLEHOSTD_STACK_SIZE     (OS_STACK_ALIGN(512))
#define BLEHOSTD_TASK_PRIO      3

#define BLEHOSTD_MAX_MSG_SZ     10240

static FILE *blehostd_log_file;

static struct os_task blehostd_task;
static os_stack_t blehostd_stack[BLEHOSTD_STACK_SIZE];

static struct os_mqueue blehostd_req_mq;
static struct os_mqueue blehostd_rsp_mq;

static struct mn_sockaddr_un blehostd_server_addr;
static struct mn_socket *blehostd_socket;

static const char *blehostd_socket_filename;
static const char *blehostd_dev_filename;

static struct os_mbuf *blehostd_packet;
static uint16_t blehostd_packet_len;

void
blehostd_logf(const char *fmt, ...)
{
    static char buf[10240];
    static int midline;
	va_list args;
    time_t tt;
    char *ts;
    int off;

    if (blehostd_log_file == NULL) {
        return;
    }

    off = 0;

	va_start(args, fmt);

    if (!midline) {
        time(&tt);
        ts = ctime(&tt);
        if (ts != NULL) {
            ts[24] = '\0';
            off += snprintf(buf + off, sizeof buf - off, "%s    ", ts);
        }
    }

    off += vsnprintf(buf + off, sizeof buf - off, fmt, args);
    va_end(args);

    if (off <= sizeof buf) {
        midline = buf[off - 1] != '\n';
    }

    fputs(buf, blehostd_log_file);
}

int
blehostd_enqueue_rsp(const char *json_rsp)
{
    struct os_mbuf *om;
    uint16_t *rsplen;
    size_t len;
    int rc;

    om = NULL;

    len = strlen(json_rsp);
    if (len > BLEHOSTD_MAX_MSG_SZ) {
        rc = SYS_EINVAL;
        goto err;
    }

    om = os_msys_get_pkthdr(len + 2, 0);
    if (om == NULL) {
        rc = SYS_ENOMEM;
        goto err;
    }

    rsplen = os_mbuf_extend(om, sizeof *rsplen);
    if (rsplen == NULL) {
        rc = SYS_ENOMEM;
        goto err;
    }

    *rsplen = htons(len);

    rc = os_mbuf_append(om, json_rsp, len);
    if (rc != 0) {
        rc = SYS_ENOMEM;
        goto err;
    }

    rc = os_mqueue_put(&blehostd_rsp_mq, os_eventq_dflt_get(), om);
    assert(rc == 0);

    return 0;

err:
    os_mbuf_free_chain(om);
    return rc;
}

static void
blehostd_process_rsp(struct os_mbuf *om)
{
    uint8_t buf[4096];
    int rc;
    int i;

    assert(sizeof buf >= OS_MBUF_PKTLEN(om));
    BHD_LOG(DEBUG, "Sending %d bytes\n", OS_MBUF_PKTLEN(om));

    if (MYNEWT_VAL(LOG_LEVEL) <= LOG_LEVEL_DEBUG) {
        rc = os_mbuf_copydata(om, 0, OS_MBUF_PKTLEN(om), buf);
        assert(rc == 0);
        for (i = 0; i < OS_MBUF_PKTLEN(om); i++) {
            BHD_LOG(DEBUG, "%s0x%02x", i == 0 ? "" : " ", buf[i]);
        }
        BHD_LOG(DEBUG, "\n");
    }

    rc = mn_sendto(blehostd_socket, om,
                   (struct mn_sockaddr *)&blehostd_server_addr);
    if (rc != 0) {
        BHD_LOG(INFO, "mn_sendto() failed; rc=%d\n", rc);
    }
}

static void
blehostd_process_rsp_mq(struct os_event *ev)
{
    struct os_mbuf *om;

    while ((om = os_mqueue_get(&blehostd_rsp_mq)) != NULL) {
        blehostd_process_rsp(om);
    }
}

static int
blehostd_fill_addr(struct mn_sockaddr_un *addr, const char *filename)
{
    size_t name_len;

    name_len = strlen(filename);
    if (name_len + 1 > sizeof addr->msun_path) {
        return -1;
    }

    addr->msun_len = sizeof *addr;
    addr->msun_family = MN_AF_LOCAL;
    memcpy(addr->msun_path, filename, name_len + 1);

    return 0;
}

static int
blehostd_connect(const char *sock_path)
{
    int rc;

    rc = blehostd_fill_addr(&blehostd_server_addr, sock_path);
    if (rc != 0) {
        return rc;
    }

    rc = mn_socket(&blehostd_socket, MN_PF_LOCAL, SOCK_STREAM, 0);
    if (rc != 0) {
        return -1;
    }

    rc = mn_connect(blehostd_socket,
                    (struct mn_sockaddr *)&blehostd_server_addr);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

static int
blehostd_enqueue_one(void)
{
    struct os_mbuf *om;
    int rc;

    if (blehostd_packet_len == 0) {
        rc = os_mbuf_copydata(blehostd_packet, 0, sizeof blehostd_packet_len,
                              &blehostd_packet_len);
        if (rc == 0) {
            blehostd_packet_len = ntohs(blehostd_packet_len);

            /* Temporary hack: Allow user to bypass length header; assume
             * entire packet received in one read.
             */
            if (blehostd_packet_len > BLEHOSTD_MAX_MSG_SZ) {
                blehostd_packet_len = OS_MBUF_PKTLEN(blehostd_packet);
            } else {
                os_mbuf_adj(blehostd_packet, sizeof blehostd_packet_len);
            }
        }
    }

    if (blehostd_packet_len == 0 ||
        OS_MBUF_PKTLEN(blehostd_packet) < blehostd_packet_len) {

        return 0;
    }

    if (OS_MBUF_PKTLEN(blehostd_packet) == blehostd_packet_len) {
        om = blehostd_packet;
        blehostd_packet = NULL;
        blehostd_packet_len = 0;
    } else {
        /* Full packet plus some (or all) of next packet received. */
        om = os_msys_get_pkthdr(blehostd_packet_len, 0);
        if (om == NULL) {
            fprintf(stderr, "* Error: failed to allocate mbuf\n");
            return 0;
        }

        rc = os_mbuf_appendfrom(om, blehostd_packet, 0, blehostd_packet_len);
        if (rc != 0) {
            fprintf(stderr, "* Error: failed to allocate mbuf\n");
            return 0;
        }

        os_mbuf_adj(blehostd_packet, blehostd_packet_len);
        blehostd_packet_len = 0;
    }

    rc = os_mqueue_put(&blehostd_req_mq, os_eventq_dflt_get(), om);
    assert(rc == 0);

    return 1;
}

static int
blehostd_recv_packet(void)
{
    struct mn_sockaddr_un from_addr;
    struct os_mbuf *om;
    int enqueued;
    int rc;

    rc = mn_recvfrom(blehostd_socket, &om, (void *)&from_addr);
    if (rc != 0) {
        /* (very) poor man's blocking receive. */
        os_time_delay(1);
        return -1;
    }

    if (blehostd_packet == NULL) {
        /* Beginning of packet. */
        blehostd_packet = om;
    } else {
        /* Continuation of packet. */
        os_mbuf_concat(blehostd_packet, om);
    }

    while (1) {
        enqueued = blehostd_enqueue_one();
        if (!enqueued) {
            break;
        }
    }

    return 0;
}

static void
blehostd_task_handler(void *arg)
{
    int rc;

    rc = blehostd_connect(blehostd_socket_filename);
    if (rc != 0) {
        assert(0);
    }

    while (1) {
        blehostd_recv_packet();
    }
}

static void
blehostd_process_req(struct os_mbuf *om)
{
    struct bhd_rsp rsp = {{0}};
    char *json;
    int send_rsp;
    int rc;

    json = malloc(OS_MBUF_PKTLEN(om) + 1);
    if (json == NULL) {
        BHD_LOG(CRITICAL, "OOM\n");
        goto done;
    }

    BHD_LOG(DEBUG, "Received %d bytes:\n%s\n", OS_MBUF_PKTLEN(om), json);
    rc = os_mbuf_copydata(om, 0, OS_MBUF_PKTLEN(om), json);
    if (rc != 0) {
        BHD_LOG(ERROR, "os_mbuf_copydata() failed: rc=%d\n", rc);
        goto done;
    }

    BHD_LOG(DEBUG, "Received JSON request:\n%s\n", json);

    send_rsp = bhd_req_dec(json, &rsp);
    if (send_rsp) {
        bhd_rsp_send(&rsp);
    }

done:
    os_mbuf_free_chain(om);
}

static void
blehostd_process_req_mq(struct os_event *ev)
{
    struct os_mbuf *om;

    while ((om = os_mqueue_get(&blehostd_req_mq)) != NULL) {
        blehostd_process_req(om);
    }
}

static void
blehostd_on_sync(void)
{
    int rc;

    rc = bhd_send_sync_evt(bhd_next_evt_seq());
    assert(rc == 0);
}

static void
print_usage(FILE *stream)
{
    fprintf(stream, "usage: blehostd <controller-device> <socket-path>\n");
}

static void
bhd_open_log(char *argv0)
{
    size_t len;

    len = strlen(argv0);
    if (len < 3) {
        return;
    }

    argv0[len - 3] = 'l';
    argv0[len - 2] = 'o';
    argv0[len - 1] = 'g';
    blehostd_log_file = fopen(argv0, "a");
}

int
main(int argc, char **argv)
{
    sigset_t sigset;
    int rc;

    if (!g_os_started) {
        if (argc < 3) {
            print_usage(stderr);
            return EXIT_FAILURE;
        }

        bhd_open_log(argv[0]);

        blehostd_dev_filename = argv[1];
        blehostd_socket_filename = argv[2];

        BHD_LOG(INFO,
                "*** Starting blehostd %s %s\n",
                blehostd_dev_filename, blehostd_socket_filename);

#ifdef ARCH_sim
        mcu_sim_parse_args(1, argv);
#endif
    }

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);

    errno = 0;
    rc = sigprocmask(SIG_BLOCK, &sigset, NULL);
    if (rc != 0) {
        fprintf(stderr, "sigprocmask failed; %d (%s)\n",
                errno, strerror(errno));
        return EXIT_FAILURE;
    }

    rc = uart_set_dev(MYNEWT_VAL(BLE_HCI_UART_PORT), blehostd_dev_filename);
    if (rc != 0) {
        fprintf(stderr, "Failed to open a serial connection to %s\n",
                blehostd_dev_filename);
        return EXIT_FAILURE;
    }

    sysinit();

    rc = os_mqueue_init(&blehostd_req_mq, blehostd_process_req_mq, NULL);
    assert(rc == 0);

    rc = os_mqueue_init(&blehostd_rsp_mq, blehostd_process_rsp_mq, NULL);
    assert(rc == 0);

    os_task_init(&blehostd_task, "blehostd", blehostd_task_handler,
                 NULL, BLEHOSTD_TASK_PRIO, OS_WAIT_FOREVER,
                 blehostd_stack, BLEHOSTD_STACK_SIZE);

    ble_hs_evq_set(os_eventq_dflt_get());
    ble_hs_cfg.sync_cb = blehostd_on_sync;

    while (1) {
        os_eventq_run(os_eventq_dflt_get());
    }

    return 0;
}

#ifndef H_BHD_PROTO_
#define H_BHD_PROTO_

#include <inttypes.h>
#include "blehostd.h"
#include "host/ble_hs.h"

#define BHD_MSG_OP_REQ                      1
#define BHD_MSG_OP_RSP                      2
#define BHD_MSG_OP_EVT                      3
            
#define BHD_MSG_TYPE_ERR                    1
#define BHD_MSG_TYPE_SYNC                   2
#define BHD_MSG_TYPE_CONNECT                3
#define BHD_MSG_TYPE_TERMINATE              4
#define BHD_MSG_TYPE_DISC_ALL_SVCS          5
#define BHD_MSG_TYPE_DISC_SVC_UUID          6
#define BHD_MSG_TYPE_DISC_ALL_CHRS          7
#define BHD_MSG_TYPE_DISC_CHR_UUID          8
#define BHD_MSG_TYPE_WRITE                  9
#define BHD_MSG_TYPE_WRITE_CMD              10
#define BHD_MSG_TYPE_EXCHANGE_MTU           11
#define BHD_MSG_TYPE_GEN_RAND_ADDR          12
#define BHD_MSG_TYPE_SET_RAND_ADDR          13
#define BHD_MSG_TYPE_CONN_CANCEL            14

#define BHD_MSG_TYPE_SYNC_EVT               2049
#define BHD_MSG_TYPE_CONNECT_EVT            2050
#define BHD_MSG_TYPE_DISCONNECT_EVT         2051
#define BHD_MSG_TYPE_DISC_SVC_EVT           2052
#define BHD_MSG_TYPE_DISC_CHR_EVT           2053
#define BHD_MSG_TYPE_WRITE_ACK_EVT          2054
#define BHD_MSG_TYPE_NOTIFY_RX_EVT          2055
#define BHD_MSG_TYPE_MTU_CHANGE_EVT         2056

#define BHD_ADDR_TYPE_NONE                  255

typedef uint32_t bhd_seq_t;
#define BHD_SEQ_MIN                         0
#define BHD_SEQ_MAX                         UINT32_MAX

#define BHD_EVT_SEQ_MIN                     0xffffff00

struct bhd_msg_hdr {
    int op;
    int type;
    bhd_seq_t seq;
};

struct bhd_connect_req {
    /* Mandatory. */
    int own_addr_type;
    ble_addr_t peer_addr;

    /* Optional. */
    int32_t duration_ms;
    uint16_t scan_itvl;
    uint16_t scan_window;
    uint16_t itvl_min;
    uint16_t itvl_max;
    uint16_t latency;
    uint16_t supervision_timeout;
    uint16_t min_ce_len;
    uint16_t max_ce_len;

    /* XXX: Idle timeout. */
};

struct bhd_terminate_req {
    uint16_t conn_handle;
    uint8_t hci_reason;
};

struct bhd_disc_all_svcs_req {
    uint16_t conn_handle;
};

struct bhd_disc_svc_uuid_req {
    uint16_t conn_handle;
    ble_uuid_any_t svc_uuid;
};

struct bhd_disc_all_chrs_req {
    uint16_t conn_handle;
    uint16_t start_attr_handle;
    uint16_t end_attr_handle;
};

struct bhd_disc_chr_uuid_req {
    uint16_t conn_handle;
    uint16_t start_handle;
    uint16_t end_handle;
    ble_uuid_any_t chr_uuid;
};

struct bhd_write_req {
    uint16_t conn_handle;
    uint16_t attr_handle;
    uint8_t *data;
    int data_len;
};

struct bhd_exchange_mtu_req {
    uint16_t conn_handle;
};

struct bhd_gen_rand_addr_req {
    uint8_t nrpa;
};

struct bhd_set_rand_addr_req {
    uint8_t addr[6];
};

struct bhd_req {
    struct bhd_msg_hdr hdr;
    union {
        struct bhd_connect_req connect;
        struct bhd_terminate_req terminate;
        struct bhd_disc_all_svcs_req disc_all_svcs;
        struct bhd_disc_svc_uuid_req disc_svc_uuid;
        struct bhd_disc_all_chrs_req disc_all_chrs;
        struct bhd_disc_chr_uuid_req disc_chr_uuid;
        struct bhd_write_req write;
        struct bhd_exchange_mtu_req exchange_mtu;
        struct bhd_gen_rand_addr_req gen_rand_addr;
        struct bhd_set_rand_addr_req set_rand_addr;
    };
};

struct bhd_err_rsp {
    int status;
    const char *msg;
};

struct bhd_sync_rsp {
    int synced;
};

struct bhd_connect_rsp {
    int status;
};

struct bhd_terminate_rsp {
    int status;
};

struct bhd_disc_all_svcs_rsp {
    int status;
};

struct bhd_disc_svc_uuid_rsp {
    int status;
};

struct bhd_disc_all_chrs_rsp {
    int status;
};

struct bhd_disc_chr_uuid_rsp {
    int status;
};

struct bhd_write_rsp {
    int status;
};

struct bhd_exchange_mtu_rsp {
    int status;
};

struct bhd_gen_rand_addr_rsp {
    int status;

    /* Only present if status is 0. */
    uint8_t addr[6];
};

struct bhd_set_rand_addr_rsp {
    int status;
};

struct bhd_conn_cancel_rsp {
    int status;
};

struct bhd_rsp {
    struct bhd_msg_hdr hdr;
    union {
        struct bhd_err_rsp err;
        struct bhd_sync_rsp sync;
        struct bhd_connect_rsp connect;
        struct bhd_terminate_rsp terminate;
        struct bhd_disc_all_svcs_rsp disc_all_svcs;
        struct bhd_disc_svc_uuid_rsp disc_svc_uuid;
        struct bhd_disc_all_chrs_rsp disc_all_chrs;
        struct bhd_disc_chr_uuid_rsp disc_chr_uuid;
        struct bhd_write_rsp write;
        struct bhd_exchange_mtu_rsp exchange_mtu;
        struct bhd_gen_rand_addr_rsp gen_rand_addr;
        struct bhd_set_rand_addr_rsp set_rand_addr;
        struct bhd_conn_cancel_rsp conn_cancel;
    };
};

struct bhd_sync_evt {
    int synced;
};

struct bhd_connect_evt {
    struct ble_gap_conn_desc desc;
    int status;
};

struct bhd_disconnect_evt {
    struct ble_gap_conn_desc desc;
    int reason;
};

struct bhd_disc_svc_evt {
    uint16_t conn_handle;
    int status;
    struct ble_gatt_svc svc;
};

struct bhd_disc_chr_evt {
    uint16_t conn_handle;
    int status;
    struct ble_gatt_chr chr;
};

struct bhd_write_ack_evt {
    uint16_t conn_handle;
    uint16_t attr_handle;
    int status;
};

struct bhd_notify_rx_evt {
    uint16_t conn_handle;
    uint16_t attr_handle;
    uint8_t indication:1;
    const uint8_t *data;
    int data_len;
};

struct bhd_mtu_change_evt {
    uint16_t conn_handle;
    uint16_t mtu;
    int status;
};

struct bhd_evt {
    struct bhd_msg_hdr hdr;
    union {
        struct bhd_sync_evt sync;
        struct bhd_connect_evt connect;
        struct bhd_disconnect_evt disconnect;
        struct bhd_disc_svc_evt disc_svc;
        struct bhd_disc_chr_evt disc_chr;
        struct bhd_write_ack_evt write_ack;
        struct bhd_notify_rx_evt notify_rx;
        struct bhd_mtu_change_evt mtu_change;
    };
};

int bhd_err_build(struct bhd_rsp *rsp, int status, const char *msg);

#endif

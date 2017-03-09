#include <assert.h>
#include <string.h>

#include "blehostd.h"
#include "bhd_proto.h"
#include "bhd_gap.h"
#include "bhd_util.h"
#include "defs/error.h"
#include "nimble/ble.h"
#include "host/ble_hs.h"

static const struct ble_gap_conn_desc bhd_gap_null_desc = {
    .conn_handle = BLE_HS_CONN_HANDLE_NONE,
    .our_id_addr = { BHD_ADDR_TYPE_NONE },
    .our_ota_addr = { BHD_ADDR_TYPE_NONE },
    .peer_id_addr = { BHD_ADDR_TYPE_NONE },
    .peer_ota_addr = { BHD_ADDR_TYPE_NONE },
};

int
bhd_gap_send_connect_evt(uint16_t seq, uint16_t conn_handle, int status)
{
    struct bhd_evt evt = {{0}};
    int rc;

    evt.hdr.op = BHD_MSG_OP_EVT;
    evt.hdr.type = BHD_MSG_TYPE_CONNECT_EVT;
    evt.hdr.seq = seq;
    evt.connect.status = status;

    if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        rc = ble_gap_conn_find(conn_handle, &evt.connect.desc);
        if (rc != 0) {
            return rc;
        }
    }

    rc = bhd_evt_send(&evt);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

static int
bhd_gap_send_disconnect_evt(const struct ble_gap_conn_desc *desc, int reason,
                            int_least32_t seq)
{
    struct bhd_evt evt = {{0}};
    int rc;

    evt.hdr.op = BHD_MSG_OP_EVT;
    evt.hdr.type = BHD_MSG_TYPE_DISCONNECT_EVT;
    evt.hdr.seq = seq;
    evt.disconnect.reason = reason;
    evt.disconnect.desc = *desc;

    BHD_LOG(INFO, "disconnect; handle=%d reason=%d\n",
            desc->conn_handle, reason);

    rc = bhd_evt_send(&evt);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

static int
bhd_gap_send_notify_rx_evt(uint16_t conn_handle, uint16_t attr_handle,
                            int indication,
                            const uint8_t *attr_val, int attr_len)
{
    struct bhd_evt evt = {{0}};
    int rc;

    evt.hdr.op = BHD_MSG_OP_EVT;
    evt.hdr.type = BHD_MSG_TYPE_NOTIFY_RX_EVT;
    evt.hdr.seq = 0; // XXX
    evt.notify_rx.conn_handle = conn_handle;
    evt.notify_rx.attr_handle = attr_handle;
    evt.notify_rx.indication = indication;
    evt.notify_rx.data = attr_val;
    evt.notify_rx.data_len = attr_len;

    rc = bhd_evt_send(&evt);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

static int
bhd_gap_event(struct ble_gap_event *event, void *arg)
{
    uint8_t buf[BLE_ATT_ATTR_MAX_LEN];
    uintptr_t seq;
    uint16_t attr_len;
    int rc;

    seq = (uintptr_t)arg;

    switch (event->type) {
    case BLE_GAP_EVENT_DISC:
        return 0;

    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed. */
        bhd_gap_send_connect_evt(seq,
                                 event->connect.conn_handle,
                                 event->connect.status);
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        bhd_gap_send_disconnect_evt(&event->disconnect.conn,
                                    event->disconnect.reason,
                                    seq);
        return 0;

    case BLE_GAP_EVENT_MTU:
        if (event->mtu.channel_id == BLE_L2CAP_CID_ATT) {
            bhd_send_mtu_changed(seq, event->mtu.conn_handle, 0,
                                 event->mtu.value);
        }
        return 0;

    case BLE_GAP_EVENT_NOTIFY_RX:
        rc = ble_hs_mbuf_to_flat(event->notify_rx.om, buf, sizeof buf,
                                 &attr_len);
        if (rc != 0) {
            return SYS_ERANGE;
        }

        bhd_gap_send_notify_rx_evt(event->notify_rx.conn_handle,
                                   event->notify_rx.attr_handle,
                                   event->notify_rx.indication,
                                   buf, attr_len);
        return 0;

    default:
        return 0;
    }
}

static int
bhd_gap_conn_initiate(const struct bhd_req *req)
{
    int rc;

    struct ble_gap_conn_params params = {
        .scan_itvl = req->connect.scan_itvl,
        .scan_window = req->connect.scan_window,
        .itvl_min = req->connect.itvl_min,
        .itvl_max = req->connect.itvl_max,
        .latency = req->connect.latency,
        .supervision_timeout = req->connect.supervision_timeout,
        .min_ce_len = req->connect.min_ce_len,
        .max_ce_len = req->connect.max_ce_len,
    };

    BHD_LOG(INFO, "Initiating connection to %02x:%02x:%02x:%02x:%02x:%02x "
                  "with parameters: scan_itvl=%d scan_window=%d itvl_min=%d "
                  "itvl_max=%d latency=%d supervision_timeout=%d "
                  "min_ce_len=%d max_ce_len=%d\n",
            req->connect.peer_addr.val[0],
            req->connect.peer_addr.val[1],
            req->connect.peer_addr.val[2],
            req->connect.peer_addr.val[3],
            req->connect.peer_addr.val[4],
            req->connect.peer_addr.val[5],
            params.scan_itvl,
            params.scan_window,
            params.itvl_min,
            params.itvl_max,
            params.latency,
            params.supervision_timeout,
            params.min_ce_len,
            params.max_ce_len);

    rc = ble_gap_connect(req->connect.own_addr_type,
                         &req->connect.peer_addr,
                         15000,
                         &params,
                         bhd_gap_event,
                         bhd_seq_arg(req->hdr.seq));

    return rc;
}

void
bhd_gap_connect(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    out_rsp->connect.status = bhd_gap_conn_initiate(req);
}

void
bhd_gap_terminate(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    out_rsp->terminate.status =
        ble_gap_terminate(req->terminate.conn_handle,
                          req->terminate.hci_reason);
}

void
bhd_gap_conn_cancel(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    out_rsp->conn_cancel.status = ble_gap_conn_cancel();
}

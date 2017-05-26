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

static int
bhd_gap_send_connect_evt(int status, uint16_t conn_handle, bhd_seq_t seq)
{
    struct bhd_evt evt = {{0}};
    int rc;

    evt.hdr.op = BHD_MSG_OP_EVT;
    evt.hdr.type = BHD_MSG_TYPE_CONNECT_EVT;
    evt.hdr.seq = seq;
    evt.connect.status = status;
    evt.connect.conn_handle = conn_handle;

    rc = bhd_evt_send(&evt);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

static int
bhd_gap_send_disconnect_evt(int reason,
                            const struct ble_gap_conn_desc *desc,
                            bhd_seq_t seq)
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
bhd_gap_send_notify_rx_evt(uint16_t conn_handle,
                           uint16_t attr_handle,
                           int indication,
                           const uint8_t *attr_val,
                           int attr_len)
{
    struct bhd_evt evt = {{0}};
    int rc;

    evt.hdr.op = BHD_MSG_OP_EVT;
    evt.hdr.type = BHD_MSG_TYPE_NOTIFY_RX_EVT;
    evt.hdr.seq = bhd_next_evt_seq(); 
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
bhd_gap_send_scan_evt(const struct ble_gap_disc_desc *desc, bhd_seq_t seq)
{
    struct bhd_evt evt = {{0}};
    struct ble_hs_adv_fields fields;
    int rc;
    int i;

    evt.hdr.op = BHD_MSG_OP_EVT;
    evt.hdr.type = BHD_MSG_TYPE_SCAN_EVT;
    evt.hdr.seq = seq;

    evt.scan.event_type = desc->event_type;
    evt.scan.length_data = desc->length_data;
    evt.scan.addr = desc->addr;
    evt.scan.rssi = desc->rssi;
    memcpy(evt.scan.data, desc->data, evt.scan.length_data);
    evt.scan.direct_addr = desc->direct_addr;
    
    rc = ble_hs_adv_parse_fields(&fields, evt.scan.data, evt.scan.length_data);
    if (rc == 0) {
        evt.scan.data_flags = fields.flags;

        for (i = 0; i < fields.num_uuids16; i++) {
            evt.scan.data_uuids16[i] = fields.uuids16[i].value;
        }
        evt.scan.data_num_uuids16 = fields.num_uuids16;
        evt.scan.data_uuids16_is_complete = fields.uuids16_is_complete;

        for (i = 0; i < fields.num_uuids32; i++) {
            evt.scan.data_uuids32[i] = fields.uuids32[i].value;
        }
        evt.scan.data_num_uuids32 = fields.num_uuids32;
        evt.scan.data_uuids32_is_complete = fields.uuids32_is_complete;

        for (i = 0; i < fields.num_uuids128; i++) {
            memcpy(evt.scan.data_uuids128[i], fields.uuids128[i].value, 16);
        }
        evt.scan.data_num_uuids128 = fields.num_uuids128;
        evt.scan.data_uuids128_is_complete = fields.uuids128_is_complete;

        if (fields.name != NULL) {
            memcpy(evt.scan.data_name, fields.name, fields.name_len);
            evt.scan.data_name_len = fields.name_len;
            evt.scan.data_name_is_complete = fields.name_is_complete;
        }

        evt.scan.data_tx_pwr_lvl = fields.tx_pwr_lvl;
        evt.scan.data_tx_pwr_lvl_is_present = fields.tx_pwr_lvl_is_present;

        if (fields.slave_itvl_range != NULL) {
            evt.scan.data_slave_itvl_min =
                get_be16(fields.slave_itvl_range + 0);
            evt.scan.data_slave_itvl_max =
                get_be16(fields.slave_itvl_range + 2);
            evt.scan.data_slave_itvl_range_is_present = 1;
        }

        if (fields.svc_data_uuid16 != NULL) {
            memcpy(evt.scan.data_svc_data_uuid16,
                   fields.svc_data_uuid16,
                   fields.svc_data_uuid16_len);
            evt.scan.data_svc_data_uuid16_len = fields.svc_data_uuid16_len;
        }

        for (i = 0; i < fields.num_public_tgt_addrs; i++) {
            memcpy(evt.scan.data_public_tgt_addrs[i],
                   fields.public_tgt_addr + i * 6,
                   6);
        }
        evt.scan.data_num_public_tgt_addrs = fields.num_public_tgt_addrs;

        evt.scan.data_appearance = fields.appearance;
        evt.scan.data_appearance_is_present = fields.appearance_is_present;

        evt.scan.data_adv_itvl = fields.adv_itvl;
        evt.scan.data_adv_itvl_is_present = fields.adv_itvl_is_present;

        if (fields.svc_data_uuid32 != NULL) {
            memcpy(evt.scan.data_svc_data_uuid32,
                   fields.svc_data_uuid32,
                   fields.svc_data_uuid32_len);
            evt.scan.data_svc_data_uuid32_len = fields.svc_data_uuid32_len;
        }

        if (fields.svc_data_uuid128 != NULL) {
            memcpy(evt.scan.data_svc_data_uuid128,
                   fields.svc_data_uuid128,
                   fields.svc_data_uuid128_len);
            evt.scan.data_svc_data_uuid128_len = fields.svc_data_uuid128_len;
        }

        if (fields.uri != NULL) {
            memcpy(evt.scan.data_uri, fields.uri, fields.uri_len);
            evt.scan.data_uri_len = fields.uri_len;
        }

        if (fields.mfg_data != NULL) {
            memcpy(evt.scan.data_mfg_data,
                   fields.mfg_data,
                   fields.mfg_data_len);
            evt.scan.data_mfg_data_len = fields.mfg_data_len;
        }
    }

    rc = bhd_evt_send(&evt);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

static int
bhd_gap_send_scan_tmo_evt(bhd_seq_t seq)
{
    struct bhd_evt evt = {{0}};
    int rc;

    evt.hdr.op = BHD_MSG_OP_EVT;
    evt.hdr.type = BHD_MSG_TYPE_SCAN_TMO_EVT;
    evt.hdr.seq = seq;

    BHD_LOG(INFO, "scan_tmo\n");

    rc = bhd_evt_send(&evt);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

static int
bhd_gap_send_enc_change_evt(int status, uint16_t conn_handle, bhd_seq_t seq)
{
    struct bhd_evt evt = {{0}};
    int rc;

    evt.hdr.op = BHD_MSG_OP_EVT;
    evt.hdr.type = BHD_MSG_TYPE_ENC_CHANGE_EVT;
    evt.hdr.seq = seq;
    evt.enc_change.conn_handle = conn_handle;
    evt.enc_change.status = status;

    BHD_LOG(INFO, "enc_change; conn_handle=%d status=%d\n",
            conn_handle, status);

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
    bhd_seq_t seq;
    uint16_t attr_len;
    int rc;

    seq = (bhd_seq_t)(uintptr_t)arg;

    switch (event->type) {
    case BLE_GAP_EVENT_DISC:
        bhd_gap_send_scan_evt(&event->disc, seq);
        return 0;

    case BLE_GAP_EVENT_DISC_COMPLETE:
        bhd_gap_send_scan_tmo_evt(seq);
        return 0;

    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed. */
        bhd_gap_send_connect_evt(event->connect.status,
                                 event->connect.conn_handle,
                                 seq);
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        bhd_gap_send_disconnect_evt(event->disconnect.reason,
                                    &event->disconnect.conn,
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

    case BLE_GAP_EVENT_ENC_CHANGE:
        bhd_gap_send_enc_change_evt(event->enc_change.status,
                                    event->enc_change.conn_handle,
                                    seq);
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

void
bhd_gap_scan(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    const struct ble_gap_disc_params params = {
        .itvl = req->scan.itvl,
        .window = req->scan.window,
        .filter_policy = req->scan.filter_policy,
        .limited = req->scan.limited,
        .passive = req->scan.passive,
        .filter_duplicates = req->scan.filter_duplicates,
    };

    out_rsp->scan.status = ble_gap_disc(req->scan.own_addr_type,
                                        req->scan.duration_ms,
                                        &params,
                                        bhd_gap_event,
                                        bhd_seq_arg(req->hdr.seq));
}

void
bhd_gap_scan_cancel(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    out_rsp->scan_cancel.status = ble_gap_disc_cancel();
}

void
bhd_gap_security_initiate(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    out_rsp->security_initiate.status =
        ble_gap_security_initiate(req->security_initiate.conn_handle);
}

void
bhd_gap_conn_find(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    struct ble_gap_conn_desc desc;

    out_rsp->conn_find.status =
        ble_gap_conn_find(req->conn_find.conn_handle, &desc);
    if (out_rsp->conn_find.status != 0) {
        return;
    }

    out_rsp->conn_find.conn_handle = req->conn_find.conn_handle;
    out_rsp->conn_find.conn_itvl = desc.conn_itvl;
    out_rsp->conn_find.conn_latency = desc.conn_latency;
    out_rsp->conn_find.supervision_timeout = desc.supervision_timeout;
    out_rsp->conn_find.role = desc.role;
    out_rsp->conn_find.master_clock_accuracy = desc.master_clock_accuracy;

    out_rsp->conn_find.our_id_addr = desc.our_id_addr;
    out_rsp->conn_find.peer_id_addr = desc.peer_id_addr;
    out_rsp->conn_find.our_ota_addr = desc.our_ota_addr;
    out_rsp->conn_find.peer_ota_addr = desc.peer_ota_addr;

    out_rsp->conn_find.encrypted = desc.sec_state.encrypted;
    out_rsp->conn_find.authenticated = desc.sec_state.authenticated;
    out_rsp->conn_find.bonded = desc.sec_state.bonded;
    out_rsp->conn_find.key_size = desc.sec_state.key_size;
}

void
bhd_gap_adv_start(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    struct ble_gap_adv_params adv_params;

    adv_params.conn_mode = req->adv_start.conn_mode;
    adv_params.disc_mode = req->adv_start.disc_mode;
    adv_params.itvl_min = req->adv_start.itvl_min;
    adv_params.itvl_max = req->adv_start.itvl_max;
    adv_params.channel_map = req->adv_start.channel_map;
    adv_params.filter_policy = req->adv_start.filter_policy;
    adv_params.high_duty_cycle = req->adv_start.high_duty_cycle;

    out_rsp->adv_start.status = ble_gap_adv_start(req->adv_start.own_addr_type,
                                                  &req->adv_start.peer_addr,
                                                  req->adv_start.duration_ms,
                                                  &adv_params,
                                                  bhd_gap_event,
                                                  bhd_seq_arg(req->hdr.seq));
}

void
bhd_gap_adv_stop(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    out_rsp->adv_stop.status = ble_gap_adv_stop();
}

void
bhd_gap_adv_set_data(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    out_rsp->adv_set_data.status =
        ble_gap_adv_set_data(req->adv_set_data.data,
                             req->adv_set_data.data_len);

}

void
bhd_gap_adv_rsp_set_data(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    out_rsp->adv_rsp_set_data.status =
        ble_gap_adv_rsp_set_data(req->adv_rsp_set_data.data,
                                 req->adv_rsp_set_data.data_len);

}

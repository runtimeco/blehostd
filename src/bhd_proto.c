#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "blehostd.h"
#include "bhd_proto.h"
#include "bhd_gattc.h"
#include "bhd_gap.h"
#include "bhd_util.h"
#include "bhd_id.h"
#include "parse.h"
#include "defs/error.h"
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "cjson/cJSON.h"

typedef int bhd_req_handle_fn(cJSON *parent, struct bhd_req *req,
                              struct bhd_rsp *rsp);
static bhd_req_handle_fn bhd_sync_req_handle;
static bhd_req_handle_fn bhd_connect_req_handle;
static bhd_req_handle_fn bhd_terminate_req_handle;
static bhd_req_handle_fn bhd_disc_all_svcs_req_handle;
static bhd_req_handle_fn bhd_disc_svc_uuid_req_handle;
static bhd_req_handle_fn bhd_disc_all_chrs_req_handle;
static bhd_req_handle_fn bhd_disc_chr_uuid_req_handle;
static bhd_req_handle_fn bhd_write_req_handle;
static bhd_req_handle_fn bhd_write_cmd_req_handle;
static bhd_req_handle_fn bhd_exchange_mtu_req_handle;
static bhd_req_handle_fn bhd_gen_rand_addr_handle;
static bhd_req_handle_fn bhd_set_rand_addr_handle;
static bhd_req_handle_fn bhd_conn_cancel_handle;

static const struct bhd_req_dispatch_entry {
    int req_type;
    bhd_req_handle_fn *cb;
} bhd_req_dispatch[] = {
    { BHD_MSG_TYPE_SYNC, bhd_sync_req_handle },
    { BHD_MSG_TYPE_CONNECT, bhd_connect_req_handle },
    { BHD_MSG_TYPE_TERMINATE, bhd_terminate_req_handle },
    { BHD_MSG_TYPE_DISC_ALL_SVCS, bhd_disc_all_svcs_req_handle },
    { BHD_MSG_TYPE_DISC_SVC_UUID, bhd_disc_svc_uuid_req_handle },
    { BHD_MSG_TYPE_DISC_ALL_CHRS, bhd_disc_all_chrs_req_handle },
    { BHD_MSG_TYPE_DISC_CHR_UUID, bhd_disc_chr_uuid_req_handle },
    { BHD_MSG_TYPE_WRITE, bhd_write_req_handle },
    { BHD_MSG_TYPE_WRITE_CMD, bhd_write_cmd_req_handle },
    { BHD_MSG_TYPE_EXCHANGE_MTU, bhd_exchange_mtu_req_handle },
    { BHD_MSG_TYPE_GEN_RAND_ADDR, bhd_gen_rand_addr_handle },
    { BHD_MSG_TYPE_SET_RAND_ADDR, bhd_set_rand_addr_handle },
    { BHD_MSG_TYPE_CONN_CANCEL, bhd_conn_cancel_handle },

    { -1 },
};

typedef int bhd_subrsp_enc_fn(cJSON *parent, const struct bhd_rsp *rsp);
static bhd_subrsp_enc_fn bhd_err_rsp_enc;
static bhd_subrsp_enc_fn bhd_sync_rsp_enc;
static bhd_subrsp_enc_fn bhd_connect_rsp_enc;
static bhd_subrsp_enc_fn bhd_terminate_rsp_enc;
static bhd_subrsp_enc_fn bhd_disc_all_svcs_rsp_enc;
static bhd_subrsp_enc_fn bhd_disc_svc_uuid_rsp_enc;
static bhd_subrsp_enc_fn bhd_disc_all_chrs_rsp_enc;
static bhd_subrsp_enc_fn bhd_disc_chr_uuid_rsp_enc;
static bhd_subrsp_enc_fn bhd_write_rsp_enc;
static bhd_subrsp_enc_fn bhd_exchange_mtu_rsp_enc;
static bhd_subrsp_enc_fn bhd_gen_rand_addr_rsp_enc;
static bhd_subrsp_enc_fn bhd_set_rand_addr_rsp_enc;
static bhd_subrsp_enc_fn bhd_conn_cancel_rsp_enc;

static const struct bhd_rsp_dispatch_entry {
    int rsp_type;
    bhd_subrsp_enc_fn *cb;
} bhd_rsp_dispatch[] = {
    { BHD_MSG_TYPE_ERR, bhd_err_rsp_enc },
    { BHD_MSG_TYPE_SYNC, bhd_sync_rsp_enc },
    { BHD_MSG_TYPE_CONNECT, bhd_connect_rsp_enc },
    { BHD_MSG_TYPE_TERMINATE, bhd_terminate_rsp_enc },
    { BHD_MSG_TYPE_DISC_ALL_SVCS, bhd_disc_all_svcs_rsp_enc },
    { BHD_MSG_TYPE_DISC_SVC_UUID, bhd_disc_svc_uuid_rsp_enc },
    { BHD_MSG_TYPE_DISC_ALL_CHRS, bhd_disc_all_chrs_rsp_enc },
    { BHD_MSG_TYPE_DISC_CHR_UUID, bhd_disc_chr_uuid_rsp_enc },
    { BHD_MSG_TYPE_WRITE, bhd_write_rsp_enc },
    { BHD_MSG_TYPE_WRITE_CMD, bhd_write_rsp_enc },
    { BHD_MSG_TYPE_EXCHANGE_MTU, bhd_exchange_mtu_rsp_enc },
    { BHD_MSG_TYPE_GEN_RAND_ADDR, bhd_gen_rand_addr_rsp_enc },
    { BHD_MSG_TYPE_SET_RAND_ADDR, bhd_set_rand_addr_rsp_enc },
    { BHD_MSG_TYPE_CONN_CANCEL, bhd_conn_cancel_rsp_enc },

    { -1 },
};

typedef int bhd_evt_enc_fn(cJSON *parent, const struct bhd_evt *evt);
static bhd_evt_enc_fn bhd_sync_evt_enc;
static bhd_evt_enc_fn bhd_connect_evt_enc;
static bhd_evt_enc_fn bhd_disconnect_evt_enc;
static bhd_evt_enc_fn bhd_disc_svc_evt_enc;
static bhd_evt_enc_fn bhd_disc_chr_evt_enc;
static bhd_evt_enc_fn bhd_write_ack_evt_enc;
static bhd_evt_enc_fn bhd_notify_rx_evt_enc;
static bhd_evt_enc_fn bhd_mtu_change_evt_enc;

static const struct bhd_evt_dispatch_entry {
    int msg_type;
    bhd_evt_enc_fn *cb;
} bhd_evt_dispatch[] = {
    { BHD_MSG_TYPE_SYNC_EVT,        bhd_sync_evt_enc },
    { BHD_MSG_TYPE_CONNECT_EVT,     bhd_connect_evt_enc },
    { BHD_MSG_TYPE_DISCONNECT_EVT,  bhd_disconnect_evt_enc },
    { BHD_MSG_TYPE_DISC_SVC_EVT,    bhd_disc_svc_evt_enc },
    { BHD_MSG_TYPE_DISC_CHR_EVT,    bhd_disc_chr_evt_enc },
    { BHD_MSG_TYPE_WRITE_ACK_EVT,   bhd_write_ack_evt_enc },
    { BHD_MSG_TYPE_NOTIFY_RX_EVT,   bhd_notify_rx_evt_enc },
    { BHD_MSG_TYPE_MTU_CHANGE_EVT,  bhd_mtu_change_evt_enc },

    { -1 },
};

static bhd_req_handle_fn *
bhd_req_dispatch_find(int req_type)
{
    const struct bhd_req_dispatch_entry *entry;

    for (entry = bhd_req_dispatch; entry->req_type != -1; entry++) {
        if (entry->req_type == req_type) {
            return entry->cb;
        }
    }

    return NULL;
}

static bhd_subrsp_enc_fn *
bhd_rsp_dispatch_find(int rsp_type)
{
    const struct bhd_rsp_dispatch_entry *entry;

    for (entry = bhd_rsp_dispatch; entry->rsp_type != -1; entry++) {
        if (entry->rsp_type == rsp_type) {
            return entry->cb;
        }
    }

    assert(0);
    return NULL;
}

static bhd_evt_enc_fn *
bhd_evt_dispatch_find(int evt_type)
{
    const struct bhd_evt_dispatch_entry *entry;

    for (entry = bhd_evt_dispatch; entry->msg_type != -1; entry++) {
        if (entry->msg_type == evt_type) {
            return entry->cb;
        }
    }

    assert(0);
    return NULL;
}

static int
bhd_err_fill(struct bhd_err_rsp *err, int status, const char *msg)
{
    err->status = status;
    err->msg = msg;
    return status;
}

int
bhd_err_build(struct bhd_rsp *rsp, int status, const char *msg)
{
    rsp->hdr.op = BHD_MSG_OP_RSP;
    rsp->hdr.type = BHD_MSG_TYPE_ERR;
    rsp->err.status = status;
    rsp->err.msg = msg;
    return status;
}

static struct bhd_err_rsp
bhd_msg_hdr_dec(cJSON *parent, struct bhd_msg_hdr *hdr)
{
    struct bhd_err_rsp err = {0};
    char *valstr;
    int rc;

    valstr = bhd_json_string(parent, "op", &rc);
    if (rc != 0) {
        bhd_err_fill(&err, rc, "invalid op");
        return err;
    }
    hdr->op = bhd_op_parse(valstr);
    if (hdr->op == -1) {
        bhd_err_fill(&err, SYS_ERANGE, "invalid op");
        return err;
    }

    valstr = bhd_json_string(parent, "type", &rc);
    if (rc != 0) {
        bhd_err_fill(&err, rc, "invalid type");
        return err;
    }
    hdr->type = bhd_type_parse(valstr);
    if (hdr->type == -1) {
        bhd_err_fill(&err, SYS_ERANGE, "invalid type");
        return err;
    }

    hdr->seq = bhd_json_int_bounds(parent, "seq", 0, BHD_SEQ_MAX, &rc);
    if (rc != 0) {
        bhd_err_fill(&err, rc, "invalid seq");
        return err;
    }

    return err;
}

static int
bhd_msg_hdr_enc(cJSON *parent, const struct bhd_msg_hdr *hdr)
{
    cJSON_AddStringToObject(parent, "op", bhd_op_rev_parse(hdr->op));
    cJSON_AddStringToObject(parent, "type", bhd_type_rev_parse(hdr->type));
    bhd_json_add_int(parent, "seq", hdr->seq);

    return 0;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_sync_req_handle(cJSON *parent, struct bhd_req *req, struct bhd_rsp *rsp)
{
    rsp->sync.synced = ble_hs_synced();
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_connect_req_handle(cJSON *parent, struct bhd_req *req, struct bhd_rsp *rsp)
{
    int rc;

    req->connect.own_addr_type =
        bhd_json_addr_type(parent, "own_addr_type", &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid own_addr_type");
        return 1;
    }

    req->connect.peer_addr.type =
        bhd_json_addr_type(parent, "peer_addr_type", &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid peer_addr_type");
        return 1;
    }

    bhd_json_addr(parent, "peer_addr", req->connect.peer_addr.val, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid peer_addr");
        return 1;
    }

    req->connect.duration_ms =
        bhd_json_int_bounds(parent, "duration_ms", 0, INT32_MAX, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid duration_ms");
        return 1;
    }

    req->connect.scan_itvl =
        bhd_json_int_bounds(parent, "scan_itvl", 0, INT16_MAX, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid scan_itvl");
        return 1;
    }

    req->connect.scan_window =
        bhd_json_int_bounds(parent, "scan_window", 0, INT16_MAX, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid scan_window");
        return 1;
    }

    req->connect.itvl_min =
        bhd_json_int_bounds(parent, "itvl_min", 0, INT16_MAX, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid itvl_min");
        return 1;
    }

    req->connect.itvl_max =
        bhd_json_int_bounds(parent, "itvl_max", 0, INT16_MAX, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid itvl_max");
        return 1;
    }

    req->connect.latency =
        bhd_json_int_bounds(parent, "latency", 0, INT16_MAX, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid latency");
        return 1;
    }

    req->connect.supervision_timeout =
        bhd_json_int_bounds(parent, "supervision_timeout", 0, INT16_MAX, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid supervision_timeout");
        return 1;
    }

    req->connect.min_ce_len =
        bhd_json_int_bounds(parent, "min_ce_len", 0, INT16_MAX, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid min_ce_len");
        return 1;
    }

    req->connect.max_ce_len =
        bhd_json_int_bounds(parent, "max_ce_len", 0, INT16_MAX, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid max_ce_len");
        return 1;
    }

    bhd_gap_connect(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_terminate_req_handle(cJSON *parent,
                         struct bhd_req *req, struct bhd_rsp *rsp)
{
    int rc;

    req->terminate.conn_handle =
        bhd_json_int_bounds(parent, "conn_handle", 0, 0xffff, &rc);
    if (rc != 0) {
        bhd_err_fill(&rsp->err, rc, "invalid conn_handle");
        return 1;
    }

    req->terminate.hci_reason =
        bhd_json_int_bounds(parent, "hci_reason", 0, 0xff, &rc);
    if (rc != 0) {
        bhd_err_fill(&rsp->err, rc, "invalid hci_reason");
        return 1;
    }

    bhd_gap_terminate(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_disc_all_svcs_req_handle(cJSON *parent,
                             struct bhd_req *req, struct bhd_rsp *rsp)
{
    int rc;

    req->disc_all_svcs.conn_handle =
        bhd_json_int_bounds(parent, "conn_handle", 0, 0xffff, &rc);
    if (rc != 0) {
        bhd_err_fill(&rsp->err, rc, "invalid conn_handle");
        return 1;
    }

    bhd_gattc_disc_all_svcs(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_disc_svc_uuid_req_handle(cJSON *parent,
                             struct bhd_req *req, struct bhd_rsp *rsp)
{
    int rc;

    req->disc_svc_uuid.conn_handle =
        bhd_json_int_bounds(parent, "conn_handle", 0, 0xffff, &rc);
    if (rc != 0) {
        bhd_err_fill(&rsp->err, rc, "invalid conn_handle");
        return 1;
    }

    bhd_json_uuid(parent, "svc_uuid", &req->disc_svc_uuid.svc_uuid, &rc);
    if (rc != 0) {
        bhd_err_fill(&rsp->err, rc, "invalid svc_uuid");
        return 1;
    }

    bhd_gattc_disc_svc_uuid(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_disc_all_chrs_req_handle(cJSON *parent,
                             struct bhd_req *req, struct bhd_rsp *rsp)
{
    int rc;

    req->disc_all_chrs.conn_handle =
        bhd_json_int_bounds(parent, "conn_handle", 0, 0xffff, &rc);
    if (rc != 0) {
        bhd_err_fill(&rsp->err, rc, "invalid conn_handle");
        return 1;
    }

    req->disc_all_chrs.start_attr_handle =
        bhd_json_int_bounds(parent, "start_handle", 0, 0xffff, &rc);
    if (rc != 0) {
        bhd_err_fill(&rsp->err, rc, "invalid start_attr_handle");
        return 1;
    }

    req->disc_all_chrs.end_attr_handle =
        bhd_json_int_bounds(parent, "end_handle", 0, 0xffff, &rc);
    if (rc != 0) {
        bhd_err_fill(&rsp->err, rc, "invalid end_attr_handle");
        return 1;
    }

    bhd_gattc_disc_all_chrs(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_disc_chr_uuid_req_handle(cJSON *parent,
                             struct bhd_req *req, struct bhd_rsp *rsp)
{
    int rc;

    req->disc_svc_uuid.conn_handle =
        bhd_json_int_bounds(parent, "conn_handle", 0, 0xffff, &rc);
    if (rc != 0) {
        bhd_err_fill(&rsp->err, rc, "invalid conn_handle");
        return 1;
    }

    req->disc_chr_uuid.start_handle =
        bhd_json_int_bounds(parent, "start_handle", 0, 0xffff, &rc);
    if (rc != 0) {
        bhd_err_fill(&rsp->err, rc, "invalid start_attr_handle");
        return 1;
    }

    req->disc_chr_uuid.end_handle =
        bhd_json_int_bounds(parent, "end_handle", 0, 0xffff, &rc);
    if (rc != 0) {
        bhd_err_fill(&rsp->err, rc, "invalid end_attr_handle");
        return 1;
    }

    bhd_json_uuid(parent, "chr_uuid", &req->disc_chr_uuid.chr_uuid, &rc);
    if (rc != 0) {
        bhd_err_fill(&rsp->err, rc, "invalid chr_uuid");
        return 1;
    }

    bhd_gattc_disc_chr_uuid(req, rsp);
    return 1;
}

/**
 * @return                      0 on success; nonzero on failure.
 */
static int
bhd_write_req_dec(cJSON *parent, uint8_t *attr_buf,
                  struct bhd_req *req, struct bhd_rsp *rsp)
{
    int rc;

    req->write.conn_handle =
        bhd_json_int_bounds(parent, "conn_handle", 0, 0xffff, &rc);
    if (rc != 0) {
        return bhd_err_fill(&rsp->err, rc, "invalid conn_handle");
    }

    req->write.attr_handle =
        bhd_json_int_bounds(parent, "attr_handle", 0, 0xffff, &rc);
    if (rc != 0) {
        return bhd_err_fill(&rsp->err, rc, "invalid attr_handle");
    }

    req->write.data = attr_buf;
    bhd_json_hex_string(parent, "data", BLE_ATT_ATTR_MAX_LEN, attr_buf,
                        &req->write.data_len, &rc);

    if (rc != 0) {
        return bhd_err_fill(&rsp->err, rc, "invalid data");
    }

    return 0;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_write_req_handle(cJSON *parent,
                     struct bhd_req *req, struct bhd_rsp *rsp)
{
    uint8_t buf[BLE_ATT_ATTR_MAX_LEN];
    int rc;

    rc = bhd_write_req_dec(parent, buf, req, rsp);
    if (rc != 0) {
        return 1;
    }

    bhd_gattc_write(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_write_cmd_req_handle(cJSON *parent,
                         struct bhd_req *req, struct bhd_rsp *rsp)
{
    uint8_t buf[BLE_ATT_ATTR_MAX_LEN];
    int rc;

    rc = bhd_write_req_dec(parent, buf, req, rsp);
    if (rc != 0) {
        return 1;
    }

    bhd_gattc_write_no_rsp(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_exchange_mtu_req_handle(cJSON *parent, struct bhd_req *req,
                            struct bhd_rsp *rsp)
{
    int rc;

    req->exchange_mtu.conn_handle =
        bhd_json_int_bounds(parent, "conn_handle", 0, 0xffff, &rc);
    if (rc != 0) {
        return bhd_err_fill(&rsp->err, rc, "invalid conn_handle");
    }

    bhd_gattc_exchange_mtu(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_gen_rand_addr_handle(cJSON *parent, struct bhd_req *req,
                         struct bhd_rsp *rsp)
{
    int rc;

    req->gen_rand_addr.nrpa =
        bhd_json_bool(parent, "nrpa", &rc);
    if (rc != 0) {
        return bhd_err_fill(&rsp->err, rc, "invalid nrpa value");
    }

    bhd_id_gen_rand_addr(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_set_rand_addr_handle(cJSON *parent, struct bhd_req *req,
                         struct bhd_rsp *rsp)
{
    int rc;

    bhd_json_addr(parent, "addr", req->set_rand_addr.addr, &rc);
    if (rc != 0) {
        return bhd_err_fill(&rsp->err, rc, "invalid address");
    }

    bhd_id_set_rand_addr(req, rsp);
    return 1;
}

static int
bhd_conn_cancel_handle(cJSON *parent, struct bhd_req *req,
                          struct bhd_rsp *rsp)
{
    bhd_gap_conn_cancel(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
int
bhd_req_dec(const char *json, struct bhd_rsp *out_rsp)
{
    bhd_req_handle_fn *req_cb;
    struct bhd_err_rsp err;
    struct bhd_req req;
    cJSON *root;
    int rc;

    /* Assume failure. */
    out_rsp->hdr.op = BHD_MSG_OP_RSP;
    out_rsp->hdr.type = BHD_MSG_TYPE_ERR;

    root = cJSON_Parse(json);
    if (root == NULL) {
        bhd_err_fill(&out_rsp->err, SYS_ERANGE, "invalid json");
        return 1;
    }

    err = bhd_msg_hdr_dec(root, &req.hdr);
    if (err.status != 0) {
        out_rsp->err = err;
        return 1;
    }
    out_rsp->hdr.seq = req.hdr.seq;

    if (req.hdr.op != BHD_MSG_OP_REQ) {
        bhd_err_fill(&out_rsp->err, SYS_ERANGE, "invalid op");
        return 1;
    }

    req_cb = bhd_req_dispatch_find(req.hdr.type);
    if (req_cb == NULL) {
        bhd_err_fill(&out_rsp->err, SYS_ERANGE, "invalid type");
        return 1;
    }

    out_rsp->hdr.type = req.hdr.type;

    rc = req_cb(root, &req, out_rsp);
    return rc;
}

static int
bhd_conn_desc_enc(cJSON *parent, const struct ble_gap_conn_desc *desc)
{
    if (desc->conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        bhd_json_add_int(parent, "conn_handle", desc->conn_handle);
    }

    if (desc->our_id_addr.type != BHD_ADDR_TYPE_NONE) {
        bhd_json_add_addr_type(parent, "own_id_addr_type",
                               desc->our_id_addr.type);
        bhd_json_add_addr(parent, "own_id_addr", desc->our_id_addr.val);
    }

    if (desc->our_ota_addr.type != BHD_ADDR_TYPE_NONE) {
        bhd_json_add_addr_type(parent, "own_ota_addr_type",
                               desc->our_ota_addr.type);
        bhd_json_add_addr(parent, "own_ota_addr", desc->our_ota_addr.val);
    }

    if (desc->peer_id_addr.type != BHD_ADDR_TYPE_NONE) {
        bhd_json_add_addr_type(parent, "peer_id_addr_type",
                               desc->peer_id_addr.type);
        bhd_json_add_addr(parent, "peer_id_addr", desc->peer_id_addr.val);
    }

    if (desc->peer_ota_addr.type != BHD_ADDR_TYPE_NONE) {
        bhd_json_add_addr_type(parent, "peer_ota_addr_type",
                               desc->peer_ota_addr.type);
        bhd_json_add_addr(parent, "peer_ota_addr", desc->peer_ota_addr.val);
    }

    return 0;
}

static int
bhd_err_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->err.status);
    cJSON_AddStringToObject(parent, "msg", rsp->err.msg);
    return 0;
}

static int
bhd_sync_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    cJSON_AddBoolToObject(parent, "synced", rsp->sync.synced);
    return 0;
}

static int
bhd_connect_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->connect.status);
    return 0;
}

static int
bhd_terminate_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->terminate.status);
    return 0;
}

static int
bhd_disc_all_svcs_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->disc_all_svcs.status);
    return 0;
}

static int
bhd_disc_svc_uuid_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->disc_svc_uuid.status);
    return 0;
}

static int
bhd_disc_all_chrs_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->disc_all_chrs.status);
    return 0;
}

static int
bhd_disc_chr_uuid_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->disc_all_chrs.status);
    return 0;
}

static int
bhd_write_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->write.status);
    return 0;
}

static int
bhd_exchange_mtu_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->exchange_mtu.status);
    return 0;
}

static int
bhd_gen_rand_addr_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->gen_rand_addr.status);
    if (rsp->gen_rand_addr.status == 0) {
        bhd_json_add_addr(parent, "addr", rsp->gen_rand_addr.addr);
    }
    return 0;
}

static int
bhd_set_rand_addr_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->set_rand_addr.status);
    return 0;
}

static int
bhd_conn_cancel_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->conn_cancel.status);
    return 0;
}

int
bhd_rsp_enc(const struct bhd_rsp *rsp, cJSON **out_root)
{
    bhd_subrsp_enc_fn *rsp_cb;
    cJSON *root;
    int rc;

    root = cJSON_CreateObject();
    if (root == NULL) {
        rc = SYS_ENOMEM;
        goto err;
    }

    rc = bhd_msg_hdr_enc(root, &rsp->hdr);
    if (rc != 0) {
        goto err;
    }

    rsp_cb = bhd_rsp_dispatch_find(rsp->hdr.type);
    if (rsp_cb == NULL) {
        rc = SYS_ERANGE;
        goto err;
    }

    rc = rsp_cb(root, rsp);
    if (rc != 0) {
        goto err;
    }

    *out_root = root;
    return 0;

err:
    cJSON_Delete(root);
    return rc;
}

static int
bhd_sync_evt_enc(cJSON *parent, const struct bhd_evt *evt)
{
    cJSON_AddBoolToObject(parent, "synced", evt->sync.synced);
    return 0;
}

static int
bhd_connect_evt_enc(cJSON *parent, const struct bhd_evt *evt)
{
    bhd_json_add_int(parent, "status", evt->connect.status);

    if (evt->connect.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        bhd_json_add_int(parent, "conn_handle", evt->connect.conn_handle);
    }

    bhd_json_add_int(parent, "conn_itvl", evt->connect.conn_itvl);
    bhd_json_add_int(parent, "conn_latency", evt->connect.conn_latency);
    bhd_json_add_int(parent, "supervision_timeout",
                     evt->connect.supervision_timeout);
    bhd_json_add_int(parent, "role", evt->connect.role);
    bhd_json_add_int(parent, "master_clock_accuracy",
                     evt->connect.master_clock_accuracy);

    if (evt->connect.our_id_addr.type != BHD_ADDR_TYPE_NONE) {
        bhd_json_add_addr_type(parent, "own_id_addr_type",
                               evt->connect.our_id_addr.type);
        bhd_json_add_addr(parent, "own_id_addr",
                          evt->connect.our_id_addr.val);
    }

    if (evt->connect.our_ota_addr.type != BHD_ADDR_TYPE_NONE) {
        bhd_json_add_addr_type(parent, "own_ota_addr_type",
                               evt->connect.our_ota_addr.type);
        bhd_json_add_addr(parent, "own_ota_addr",
                          evt->connect.our_ota_addr.val);
    }

    if (evt->connect.peer_id_addr.type != BHD_ADDR_TYPE_NONE) {
        bhd_json_add_addr_type(parent, "peer_id_addr_type",
                               evt->connect.peer_id_addr.type);
        bhd_json_add_addr(parent, "peer_id_addr",
                          evt->connect.peer_id_addr.val);
    }

    if (evt->connect.peer_ota_addr.type != BHD_ADDR_TYPE_NONE) {
        bhd_json_add_addr_type(parent, "peer_ota_addr_type",
                               evt->connect.peer_ota_addr.type);
        bhd_json_add_addr(parent, "peer_ota_addr",
                          evt->connect.peer_ota_addr.val);
    }

    bhd_json_add_bool(parent, "encrypted", evt->connect.encrypted);
    bhd_json_add_bool(parent, "authenticated", evt->connect.authenticated);
    bhd_json_add_bool(parent, "bonded", evt->connect.bonded);
    bhd_json_add_int(parent, "key_size", evt->connect.key_size);

    return 0;
}

static int
bhd_disconnect_evt_enc(cJSON *parent, const struct bhd_evt *evt)
{
    int rc;

    bhd_json_add_int(parent, "reason", evt->disconnect.reason);
    rc = bhd_conn_desc_enc(parent, &evt->disconnect.desc);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

static int
bhd_disc_svc_evt_enc(cJSON *parent, const struct bhd_evt *evt)
{
    char uuid_str[BLE_UUID_STR_LEN];
    cJSON *svc;

    bhd_json_add_int(parent, "conn_handle", evt->disc_svc.conn_handle);
    bhd_json_add_int(parent, "status", evt->disc_svc.status);

    if (evt->disc_svc.status == 0) {
        svc = cJSON_CreateObject();
        if (svc == NULL) {
            return SYS_ENOMEM;
        }

        cJSON_AddItemToObject(parent, "service", svc);

        bhd_json_add_int(svc, "start_handle", evt->disc_svc.svc.start_handle);
        bhd_json_add_int(svc, "end_handle", evt->disc_svc.svc.end_handle);

        cJSON_AddStringToObject(svc, "uuid",
                                ble_uuid_to_str(&evt->disc_svc.svc.uuid.u,
                                                uuid_str));
    }

    return 0;
}

static int
bhd_disc_chr_evt_enc(cJSON *parent, const struct bhd_evt *evt)
{
    char uuid_str[BLE_UUID_STR_LEN];
    cJSON *chr;

    bhd_json_add_int(parent, "conn_handle", evt->disc_chr.conn_handle);
    bhd_json_add_int(parent, "status", evt->disc_chr.status);

    if (evt->disc_chr.status == 0) {
        chr = cJSON_CreateObject();
        if (chr == NULL) {
            return SYS_ENOMEM;
        }

        cJSON_AddItemToObject(parent, "characteristic", chr);

        bhd_json_add_int(chr, "def_handle", evt->disc_chr.chr.def_handle);
        bhd_json_add_int(chr, "val_handle", evt->disc_chr.chr.val_handle);
        bhd_json_add_int(chr, "properties", evt->disc_chr.chr.properties);

        cJSON_AddStringToObject(chr, "uuid",
                                ble_uuid_to_str(&evt->disc_chr.chr.uuid.u,
                                                uuid_str));
    }

    return 0;
}

static int
bhd_write_ack_evt_enc(cJSON *parent, const struct bhd_evt *evt)
{
    bhd_json_add_int(parent, "conn_handle", evt->write_ack.conn_handle);
    bhd_json_add_int(parent, "attr_handle", evt->write_ack.attr_handle);
    bhd_json_add_int(parent, "status", evt->write_ack.status);
    return 0;
}

static int
bhd_notify_rx_evt_enc(cJSON *parent, const struct bhd_evt *evt)
{
    char buf[BLE_ATT_ATTR_MAX_LEN * 3];

    bhd_json_add_int(parent, "conn_handle", evt->notify_rx.conn_handle);
    bhd_json_add_int(parent, "attr_handle", evt->notify_rx.attr_handle);
    cJSON_AddBoolToObject(parent, "indication", evt->notify_rx.indication);
    cJSON_AddStringToObject(parent, "data",
                            bhd_hex_str(buf, sizeof buf, NULL,
                            evt->notify_rx.data, evt->notify_rx.data_len));
    return 0;
}

static int
bhd_mtu_change_evt_enc(cJSON *parent, const struct bhd_evt *evt)
{
    bhd_json_add_int(parent, "conn_handle", evt->mtu_change.conn_handle);
    bhd_json_add_int(parent, "mtu", evt->mtu_change.mtu);
    bhd_json_add_int(parent, "status", evt->mtu_change.status);
    return 0;
}

int
bhd_evt_enc(const struct bhd_evt *evt, cJSON **out_root)
{
    bhd_evt_enc_fn *evt_cb;
    cJSON *root;
    int rc;

    root = cJSON_CreateObject();
    if (root == NULL) {
        rc = SYS_ENOMEM;
        goto err;
    }

    rc = bhd_msg_hdr_enc(root, &evt->hdr);
    if (rc != 0) {
        goto err;
    }

    evt_cb = bhd_evt_dispatch_find(evt->hdr.type);
    if (evt_cb == NULL) {
        rc = SYS_ERANGE;
        goto err;
    }

    rc = evt_cb(root, evt);
    if (rc != 0) {
        goto err;
    }

    *out_root = root;
    return 0;

err:
    cJSON_Delete(root);
    return rc;
}

int
bhd_msg_send(cJSON *root)
{
    char *json_str;
    int rc;

    json_str = cJSON_Print(root);
    if (json_str == NULL) {
        rc = SYS_ENOMEM;
        goto done;
    }

    BHD_LOG(DEBUG, "Sending JSON over UDS:\n%s\n", json_str);

    rc = blehostd_enqueue_rsp(json_str);
    if (rc != 0) {
        goto done;
    }

done:
    cJSON_Delete(root);
    free(json_str);
    return rc;
}

int
bhd_rsp_send(const struct bhd_rsp *rsp)
{
    cJSON *root;
    int rc;

    rc = bhd_rsp_enc(rsp, &root);
    if (rc != 0) {
        return rc;
    }

    rc = bhd_msg_send(root);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

int
bhd_evt_send(const struct bhd_evt *evt)
{
    cJSON *root;
    int rc;

    rc = bhd_evt_enc(evt, &root);
    if (rc != 0) {
        return rc;
    }

    rc = bhd_msg_send(root);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

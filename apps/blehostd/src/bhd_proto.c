#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "blehostd.h"
#include "bhd_proto.h"
#include "bhd_gattc.h"
#include "bhd_gatts.h"
#include "bhd_gap.h"
#include "bhd_util.h"
#include "bhd_id.h"
#include "bhd_sm.h"
#include "parse.h"
#include "defs/error.h"
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "cjson/cJSON.h"

typedef int bhd_req_run_fn(cJSON *parent, struct bhd_req *req,
                              struct bhd_rsp *rsp);
static bhd_req_run_fn bhd_sync_req_run;
static bhd_req_run_fn bhd_connect_req_run;
static bhd_req_run_fn bhd_terminate_req_run;
static bhd_req_run_fn bhd_disc_all_svcs_req_run;
static bhd_req_run_fn bhd_disc_svc_uuid_req_run;
static bhd_req_run_fn bhd_disc_all_chrs_req_run;
static bhd_req_run_fn bhd_disc_chr_uuid_req_run;
static bhd_req_run_fn bhd_disc_all_dscs_req_run;
static bhd_req_run_fn bhd_write_req_run;
static bhd_req_run_fn bhd_write_cmd_req_run;
static bhd_req_run_fn bhd_exchange_mtu_req_run;
static bhd_req_run_fn bhd_gen_rand_addr_req_run;
static bhd_req_run_fn bhd_set_rand_addr_req_run;
static bhd_req_run_fn bhd_conn_cancel_req_run;
static bhd_req_run_fn bhd_scan_req_run;
static bhd_req_run_fn bhd_scan_cancel_req_run;
static bhd_req_run_fn bhd_set_preferred_mtu_req_run;
static bhd_req_run_fn bhd_security_initiate_req_run;
static bhd_req_run_fn bhd_conn_find_req_run;
static bhd_req_run_fn bhd_reset_req_run;
static bhd_req_run_fn bhd_adv_start_req_run;
static bhd_req_run_fn bhd_adv_stop_req_run;
static bhd_req_run_fn bhd_adv_set_data_req_run;
static bhd_req_run_fn bhd_adv_rsp_set_data_req_run;
static bhd_req_run_fn bhd_adv_fields_req_run;
static bhd_req_run_fn bhd_clear_svcs_req_run;
static bhd_req_run_fn bhd_add_svcs_req_run;
static bhd_req_run_fn bhd_commit_svcs_req_run;
static bhd_req_run_fn bhd_access_status_req_run;
static bhd_req_run_fn bhd_notify_req_run;
static bhd_req_run_fn bhd_find_chr_req_run;
static bhd_req_run_fn bhd_sm_inject_io_req_run;

static const struct bhd_req_dispatch_entry {
    int req_type;
    bhd_req_run_fn *cb;
} bhd_req_dispatch[] = {
    { BHD_MSG_TYPE_SYNC,                bhd_sync_req_run },
    { BHD_MSG_TYPE_CONNECT,             bhd_connect_req_run },
    { BHD_MSG_TYPE_TERMINATE,           bhd_terminate_req_run },
    { BHD_MSG_TYPE_DISC_ALL_SVCS,       bhd_disc_all_svcs_req_run },
    { BHD_MSG_TYPE_DISC_SVC_UUID,       bhd_disc_svc_uuid_req_run },
    { BHD_MSG_TYPE_DISC_ALL_CHRS,       bhd_disc_all_chrs_req_run },
    { BHD_MSG_TYPE_DISC_CHR_UUID,       bhd_disc_chr_uuid_req_run },
    { BHD_MSG_TYPE_DISC_ALL_DSCS,       bhd_disc_all_dscs_req_run },
    { BHD_MSG_TYPE_WRITE,               bhd_write_req_run },
    { BHD_MSG_TYPE_WRITE_CMD,           bhd_write_cmd_req_run },
    { BHD_MSG_TYPE_EXCHANGE_MTU,        bhd_exchange_mtu_req_run },
    { BHD_MSG_TYPE_GEN_RAND_ADDR,       bhd_gen_rand_addr_req_run },
    { BHD_MSG_TYPE_SET_RAND_ADDR,       bhd_set_rand_addr_req_run },
    { BHD_MSG_TYPE_CONN_CANCEL,         bhd_conn_cancel_req_run },
    { BHD_MSG_TYPE_SCAN,                bhd_scan_req_run },
    { BHD_MSG_TYPE_SCAN_CANCEL,         bhd_scan_cancel_req_run },
    { BHD_MSG_TYPE_SET_PREFERRED_MTU,   bhd_set_preferred_mtu_req_run },
    { BHD_MSG_TYPE_ENC_INITIATE,        bhd_security_initiate_req_run },
    { BHD_MSG_TYPE_CONN_FIND,           bhd_conn_find_req_run },
    { BHD_MSG_TYPE_RESET,               bhd_reset_req_run },
    { BHD_MSG_TYPE_ADV_START,           bhd_adv_start_req_run },
    { BHD_MSG_TYPE_ADV_STOP,            bhd_adv_stop_req_run },
    { BHD_MSG_TYPE_ADV_SET_DATA,        bhd_adv_set_data_req_run },
    { BHD_MSG_TYPE_ADV_RSP_SET_DATA,    bhd_adv_rsp_set_data_req_run },
    { BHD_MSG_TYPE_ADV_FIELDS,          bhd_adv_fields_req_run },
    { BHD_MSG_TYPE_CLEAR_SVCS,          bhd_clear_svcs_req_run },
    { BHD_MSG_TYPE_ADD_SVCS,            bhd_add_svcs_req_run },
    { BHD_MSG_TYPE_COMMIT_SVCS,         bhd_commit_svcs_req_run },
    { BHD_MSG_TYPE_ACCESS_STATUS,       bhd_access_status_req_run },
    { BHD_MSG_TYPE_NOTIFY,              bhd_notify_req_run },
    { BHD_MSG_TYPE_FIND_CHR,            bhd_find_chr_req_run },
    { BHD_MSG_TYPE_SM_INJECT_IO,        bhd_sm_inject_io_req_run },

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
static bhd_subrsp_enc_fn bhd_disc_all_dscs_rsp_enc;
static bhd_subrsp_enc_fn bhd_write_rsp_enc;
static bhd_subrsp_enc_fn bhd_exchange_mtu_rsp_enc;
static bhd_subrsp_enc_fn bhd_gen_rand_addr_rsp_enc;
static bhd_subrsp_enc_fn bhd_set_rand_addr_rsp_enc;
static bhd_subrsp_enc_fn bhd_conn_cancel_rsp_enc;
static bhd_subrsp_enc_fn bhd_scan_rsp_enc;
static bhd_subrsp_enc_fn bhd_scan_cancel_rsp_enc;
static bhd_subrsp_enc_fn bhd_set_preferred_mtu_rsp_enc;
static bhd_subrsp_enc_fn bhd_security_initiate_rsp_enc;
static bhd_subrsp_enc_fn bhd_conn_find_rsp_enc;
static bhd_subrsp_enc_fn bhd_reset_rsp_enc;
static bhd_subrsp_enc_fn bhd_adv_start_rsp_enc;
static bhd_subrsp_enc_fn bhd_adv_stop_rsp_enc;
static bhd_subrsp_enc_fn bhd_adv_set_data_rsp_enc;
static bhd_subrsp_enc_fn bhd_adv_rsp_set_data_rsp_enc;
static bhd_subrsp_enc_fn bhd_adv_fields_rsp_enc;
static bhd_subrsp_enc_fn bhd_clear_svcs_rsp_enc;
static bhd_subrsp_enc_fn bhd_add_svcs_rsp_enc;
static bhd_subrsp_enc_fn bhd_commit_svcs_rsp_enc;
static bhd_subrsp_enc_fn bhd_access_status_rsp_enc;
static bhd_subrsp_enc_fn bhd_notify_rsp_enc;
static bhd_subrsp_enc_fn bhd_find_chr_rsp_enc;
static bhd_subrsp_enc_fn bhd_sm_inject_io_rsp_enc;

static const struct bhd_rsp_dispatch_entry {
    int rsp_type;
    bhd_subrsp_enc_fn *cb;
} bhd_rsp_dispatch[] = {
    { BHD_MSG_TYPE_ERR,                 bhd_err_rsp_enc },
    { BHD_MSG_TYPE_SYNC,                bhd_sync_rsp_enc },
    { BHD_MSG_TYPE_CONNECT,             bhd_connect_rsp_enc },
    { BHD_MSG_TYPE_TERMINATE,           bhd_terminate_rsp_enc },
    { BHD_MSG_TYPE_DISC_ALL_SVCS,       bhd_disc_all_svcs_rsp_enc },
    { BHD_MSG_TYPE_DISC_SVC_UUID,       bhd_disc_svc_uuid_rsp_enc },
    { BHD_MSG_TYPE_DISC_ALL_CHRS,       bhd_disc_all_chrs_rsp_enc },
    { BHD_MSG_TYPE_DISC_CHR_UUID,       bhd_disc_chr_uuid_rsp_enc },
    { BHD_MSG_TYPE_DISC_ALL_DSCS,       bhd_disc_all_dscs_rsp_enc },
    { BHD_MSG_TYPE_WRITE,               bhd_write_rsp_enc },
    { BHD_MSG_TYPE_WRITE_CMD,           bhd_write_rsp_enc },
    { BHD_MSG_TYPE_EXCHANGE_MTU,        bhd_exchange_mtu_rsp_enc },
    { BHD_MSG_TYPE_GEN_RAND_ADDR,       bhd_gen_rand_addr_rsp_enc },
    { BHD_MSG_TYPE_SET_RAND_ADDR,       bhd_set_rand_addr_rsp_enc },
    { BHD_MSG_TYPE_CONN_CANCEL,         bhd_conn_cancel_rsp_enc },
    { BHD_MSG_TYPE_SCAN,                bhd_scan_rsp_enc },
    { BHD_MSG_TYPE_SCAN_CANCEL,         bhd_scan_cancel_rsp_enc },
    { BHD_MSG_TYPE_SET_PREFERRED_MTU,   bhd_set_preferred_mtu_rsp_enc },
    { BHD_MSG_TYPE_ENC_INITIATE,        bhd_security_initiate_rsp_enc },
    { BHD_MSG_TYPE_CONN_FIND,           bhd_conn_find_rsp_enc },
    { BHD_MSG_TYPE_RESET,               bhd_reset_rsp_enc },
    { BHD_MSG_TYPE_ADV_START,           bhd_adv_start_rsp_enc },
    { BHD_MSG_TYPE_ADV_STOP,            bhd_adv_stop_rsp_enc },
    { BHD_MSG_TYPE_ADV_SET_DATA,        bhd_adv_set_data_rsp_enc },
    { BHD_MSG_TYPE_ADV_RSP_SET_DATA,    bhd_adv_rsp_set_data_rsp_enc },
    { BHD_MSG_TYPE_ADV_FIELDS,          bhd_adv_fields_rsp_enc },
    { BHD_MSG_TYPE_ADD_SVCS,            bhd_add_svcs_rsp_enc },
    { BHD_MSG_TYPE_CLEAR_SVCS,          bhd_clear_svcs_rsp_enc },
    { BHD_MSG_TYPE_COMMIT_SVCS,         bhd_commit_svcs_rsp_enc },
    { BHD_MSG_TYPE_ACCESS_STATUS,       bhd_access_status_rsp_enc },
    { BHD_MSG_TYPE_NOTIFY,              bhd_notify_rsp_enc },
    { BHD_MSG_TYPE_FIND_CHR,            bhd_find_chr_rsp_enc },
    { BHD_MSG_TYPE_SM_INJECT_IO,        bhd_sm_inject_io_rsp_enc },

    { -1 },
};

typedef int bhd_evt_enc_fn(cJSON *parent, const struct bhd_evt *evt);
static bhd_evt_enc_fn bhd_sync_evt_enc;
static bhd_evt_enc_fn bhd_connect_evt_enc;
static bhd_evt_enc_fn bhd_disconnect_evt_enc;
static bhd_evt_enc_fn bhd_conn_cancel_evt_enc;
static bhd_evt_enc_fn bhd_disc_svc_evt_enc;
static bhd_evt_enc_fn bhd_disc_chr_evt_enc;
static bhd_evt_enc_fn bhd_disc_dsc_evt_enc;
static bhd_evt_enc_fn bhd_write_ack_evt_enc;
static bhd_evt_enc_fn bhd_notify_rx_evt_enc;
static bhd_evt_enc_fn bhd_mtu_change_evt_enc;
static bhd_evt_enc_fn bhd_scan_tmo_evt_enc;
static bhd_evt_enc_fn bhd_scan_evt_enc;
static bhd_evt_enc_fn bhd_adv_complete_evt_enc;
static bhd_evt_enc_fn bhd_enc_change_evt_enc;
static bhd_evt_enc_fn bhd_reset_evt_enc;
static bhd_evt_enc_fn bhd_access_evt_enc;
static bhd_evt_enc_fn bhd_passkey_evt_enc;

static const struct bhd_evt_dispatch_entry {
    int msg_type;
    bhd_evt_enc_fn *cb;
} bhd_evt_dispatch[] = {
    { BHD_MSG_TYPE_SYNC_EVT,            bhd_sync_evt_enc },
    { BHD_MSG_TYPE_CONNECT_EVT,         bhd_connect_evt_enc },
    { BHD_MSG_TYPE_CONN_CANCEL_EVT,     bhd_conn_cancel_evt_enc },
    { BHD_MSG_TYPE_DISCONNECT_EVT,      bhd_disconnect_evt_enc },
    { BHD_MSG_TYPE_DISC_SVC_EVT,        bhd_disc_svc_evt_enc },
    { BHD_MSG_TYPE_DISC_CHR_EVT,        bhd_disc_chr_evt_enc },
    { BHD_MSG_TYPE_DISC_DSC_EVT,        bhd_disc_dsc_evt_enc },
    { BHD_MSG_TYPE_WRITE_ACK_EVT,       bhd_write_ack_evt_enc },
    { BHD_MSG_TYPE_NOTIFY_RX_EVT,       bhd_notify_rx_evt_enc },
    { BHD_MSG_TYPE_MTU_CHANGE_EVT,      bhd_mtu_change_evt_enc },
    { BHD_MSG_TYPE_SCAN_EVT,            bhd_scan_evt_enc },
    { BHD_MSG_TYPE_SCAN_TMO_EVT,        bhd_scan_tmo_evt_enc },
    { BHD_MSG_TYPE_ADV_COMPLETE_EVT,    bhd_adv_complete_evt_enc },
    { BHD_MSG_TYPE_ENC_CHANGE_EVT,      bhd_enc_change_evt_enc },
    { BHD_MSG_TYPE_RESET_EVT,           bhd_reset_evt_enc },
    { BHD_MSG_TYPE_ACCESS_EVT,          bhd_access_evt_enc },
    { BHD_MSG_TYPE_PASSKEY_EVT,         bhd_passkey_evt_enc },

    { -1 },
};

static bhd_req_run_fn *
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
#if MYNEWT_VAL(BLEHOSTD_FRAME_COUNTER)
    static uint32_t bhd_ctr;
#endif

    cJSON_AddStringToObject(parent, "op", bhd_op_rev_parse(hdr->op));
    cJSON_AddStringToObject(parent, "type", bhd_type_rev_parse(hdr->type));

    bhd_json_add_int(parent, "seq", hdr->seq);

#if MYNEWT_VAL(BLEHOSTD_FRAME_COUNTER)
    bhd_json_add_int(parent, "ctr", bhd_ctr++);
#endif

    return 0;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_sync_req_run(cJSON *parent, struct bhd_req *req, struct bhd_rsp *rsp)
{
    rsp->sync.synced = ble_hs_synced();
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_connect_req_run(cJSON *parent, struct bhd_req *req, struct bhd_rsp *rsp)
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
bhd_terminate_req_run(cJSON *parent,
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
bhd_disc_all_svcs_req_run(cJSON *parent,
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
bhd_disc_svc_uuid_req_run(cJSON *parent,
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
bhd_disc_all_chrs_req_run(cJSON *parent,
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
bhd_disc_chr_uuid_req_run(cJSON *parent,
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
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_disc_all_dscs_req_run(cJSON *parent,
                          struct bhd_req *req, struct bhd_rsp *rsp)
{
    int rc;

    req->disc_all_dscs.conn_handle =
        bhd_json_int_bounds(parent, "conn_handle", 0, 0xffff, &rc);
    if (rc != 0) {
        bhd_err_fill(&rsp->err, rc, "invalid conn_handle");
        return 1;
    }

    req->disc_all_dscs.start_attr_handle =
        bhd_json_int_bounds(parent, "start_handle", 0, 0xffff, &rc);
    if (rc != 0) {
        bhd_err_fill(&rsp->err, rc, "invalid start_attr_handle");
        return 1;
    }

    req->disc_all_dscs.end_attr_handle =
        bhd_json_int_bounds(parent, "end_handle", 0, 0xffff, &rc);
    if (rc != 0) {
        bhd_err_fill(&rsp->err, rc, "invalid end_attr_handle");
        return 1;
    }

    bhd_gattc_disc_all_dscs(req, rsp);
    return 1;
}

/**
 * @return                      0 on success; nonzero on failure.
 */
static int
bhd_write_req_dec(cJSON *parent, uint8_t *attr_buf, int attr_buf_sz,
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
    bhd_json_hex_string(parent, "data", attr_buf_sz, attr_buf,
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
bhd_write_req_run(cJSON *parent,
                  struct bhd_req *req, struct bhd_rsp *rsp)
{
    uint8_t buf[BLE_ATT_ATTR_MAX_LEN + 3];
    int rc;

    rc = bhd_write_req_dec(parent, buf, sizeof buf, req, rsp);
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
bhd_write_cmd_req_run(cJSON *parent,
                      struct bhd_req *req, struct bhd_rsp *rsp)
{
    uint8_t buf[BLE_ATT_ATTR_MAX_LEN + 3];
    int rc;

    rc = bhd_write_req_dec(parent, buf, sizeof buf, req, rsp);
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
bhd_exchange_mtu_req_run(cJSON *parent, struct bhd_req *req,
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
bhd_gen_rand_addr_req_run(cJSON *parent, struct bhd_req *req,
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
bhd_set_rand_addr_req_run(cJSON *parent, struct bhd_req *req,
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

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_conn_cancel_req_run(cJSON *parent, struct bhd_req *req,
                           struct bhd_rsp *rsp)
{
    bhd_gap_conn_cancel(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_scan_req_run(cJSON *parent, struct bhd_req *req, struct bhd_rsp *rsp)
{
    int rc;

    req->scan.own_addr_type =
        bhd_json_addr_type(parent, "own_addr_type", &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid own_addr_type");
        return 1;
    }

    req->scan.duration_ms =
        bhd_json_int_bounds(parent, "duration_ms", 0, INT32_MAX, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid duration_ms");
        return 1;
    }

    req->scan.itvl =
        bhd_json_int_bounds(parent, "itvl", 0, UINT16_MAX, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid itvl");
        return 1;
    }

    req->scan.window =
        bhd_json_int_bounds(parent, "window", 0, UINT16_MAX, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid window");
        return 1;
    }

    req->scan.filter_policy =
        bhd_json_scan_filter_policy(parent, "filter_policy", &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid filter_policy");
        return 1;
    }

    req->scan.limited =
        bhd_json_bool(parent, "limited", &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid limited");
        return 1;
    }

    req->scan.passive =
        bhd_json_bool(parent, "passive", &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid passive");
        return 1;
    }

    req->scan.filter_duplicates =
        bhd_json_bool(parent, "filter_duplicates", &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid filter_duplicates");
        return 1;
    }

    bhd_gap_scan(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_scan_cancel_req_run(cJSON *parent,
                        struct bhd_req *req, struct bhd_rsp *rsp)
{
    bhd_gap_scan_cancel(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_set_preferred_mtu_req_run(cJSON *parent,
                              struct bhd_req *req, struct bhd_rsp *rsp)
{
    int rc;

    req->set_preferred_mtu.mtu =
        bhd_json_int_bounds(parent, "mtu", 0, UINT16_MAX, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid mtu");
        return 1;
    }
    bhd_gattc_set_preferred_mtu(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_security_initiate_req_run(cJSON *parent,
                         struct bhd_req *req, struct bhd_rsp *rsp)
{
    int rc;

    req->security_initiate.conn_handle =
        bhd_json_int_bounds(parent, "conn_handle", 0, UINT16_MAX, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid conn_handle");
        return 1;
    }
    bhd_gap_security_initiate(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_conn_find_req_run(cJSON *parent,
                      struct bhd_req *req, struct bhd_rsp *rsp)
{
    int rc;

    req->conn_find.conn_handle =
        bhd_json_int_bounds(parent, "conn_handle", 0, UINT16_MAX, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid conn_handle");
        return 1;
    }
    bhd_gap_conn_find(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_reset_req_run(cJSON *parent,
                  struct bhd_req *req, struct bhd_rsp *rsp)
{
    ble_hs_sched_reset(0);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_adv_start_req_run(cJSON *parent,
                      struct bhd_req *req, struct bhd_rsp *rsp)
{
    int addr_type;
    int rc;

    req->adv_start = (struct bhd_adv_start_req){ 0 };

    req->adv_start.own_addr_type =
        bhd_json_addr_type(parent, "own_addr_type", &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid own_addr_type");
        return 1;
    }

    addr_type = bhd_json_addr_type(parent, "peer_addr_type", &rc);
    switch (rc) {
    case SYS_ENOENT:
        break;

    default:
        bhd_err_build(rsp, rc, "invalid peer_addr_type");
        return 1;

    case 0:
        req->adv_start.peer_addr.type = addr_type;
        bhd_json_addr(parent, "peer_addr", req->adv_start.peer_addr.val, &rc);
        if (rc != 0) {
            bhd_err_build(rsp, rc, "invalid peer_addr");
            return 1;
        }
    }

    req->adv_start.duration_ms =
        bhd_json_int_bounds(parent, "duration_ms", 0, INT32_MAX, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid duration_ms");
        return 1;
    }

    req->adv_start.conn_mode =
        bhd_json_adv_conn_mode(parent, "conn_mode", &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid conn_mode");
        return 1;
    }

    req->adv_start.disc_mode =
        bhd_json_adv_disc_mode(parent, "disc_mode", &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid disc_mode");
        return 1;
    }

    req->adv_start.itvl_min =
        bhd_json_int_bounds(parent, "itvl_min", 0, INT16_MAX, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid itvl_min");
        return 1;
    }

    req->adv_start.itvl_max =
        bhd_json_int_bounds(parent, "itvl_max", 0, INT16_MAX, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid itvl_max");
        return 1;
    }

    req->adv_start.channel_map =
        bhd_json_int_bounds(parent, "channel_map", 0, INT8_MAX, &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid channel_map");
        return 1;
    }

    req->adv_start.filter_policy =
        bhd_json_adv_filter_policy(parent, "filter_policy", &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid filter_policy");
        return 1;
    }

    req->adv_start.high_duty_cycle =
        bhd_json_bool(parent, "high_duty_cycle", &rc);
    if (rc != 0) {
        bhd_err_build(rsp, rc, "invalid high_duty_cycle");
        return 1;
    }

    bhd_gap_adv_start(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_adv_stop_req_run(cJSON *parent,
                     struct bhd_req *req, struct bhd_rsp *rsp)
{
    bhd_gap_adv_stop(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_adv_set_data_req_run(cJSON *parent,
                         struct bhd_req *req, struct bhd_rsp *rsp)
{
    int rc;

    bhd_json_hex_string(parent, "data",
                        sizeof req->adv_set_data.data, req->adv_set_data.data,
                        &req->adv_set_data.data_len, &rc);
    if (rc != 0) {
        return bhd_err_fill(&rsp->err, rc, "invalid data");
    }

    bhd_gap_adv_set_data(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_adv_rsp_set_data_req_run(cJSON *parent,
                             struct bhd_req *req, struct bhd_rsp *rsp)
{
    int rc;

    bhd_json_hex_string(parent,
                        "data",
                        sizeof req->adv_rsp_set_data.data,
                        req->adv_rsp_set_data.data,
                        &req->adv_rsp_set_data.data_len,
                        &rc);
    if (rc != 0) {
        return bhd_err_fill(&rsp->err, rc, "invalid data");
    }

    bhd_gap_adv_rsp_set_data(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_adv_fields_req_run(cJSON *parent,
                       struct bhd_req *req, struct bhd_rsp *rsp)
{
    ble_uuid_any_t uuids[sizeof req->adv_fields.uuids128 /
                         sizeof req->adv_fields.uuids128[0]];
    const char *s;
    uint8_t u8;
    int slen;
    int num_elems;
    int num_bytes;
    int rc;
    int i;

    req->adv_fields = (struct bhd_adv_fields_req){ 0 };

    u8 = bhd_json_int_bounds(parent, "flags", 0, UINT8_MAX, &rc);
    if (rc == 0) {
        if (u8 == 0) {
            bhd_err_build(rsp, SYS_EINVAL, "invalid flags");
            return 1;
        }

        req->adv_fields.flags = u8;
    } else if (rc != SYS_ENOENT) {
        bhd_err_build(rsp, rc, "invalid flags");
        return 1;
    }

    rc = ble_json_arr_uint16(parent,
                             "uuids16", 
                             sizeof req->adv_fields.uuids16 /
                               sizeof req->adv_fields.uuids16[0],
                             0,
                             0xffff,
                             req->adv_fields.uuids16,
                             &num_elems);
    if (rc == 0) {
        req->adv_fields.num_uuids16 = num_elems;
    } else if (rc != SYS_ENOENT) {
        bhd_err_build(rsp, rc, "invalid uuids16");
        return 1;
    }

    req->adv_fields.uuids16_is_complete =
        bhd_json_bool(parent, "uuids16_is_complete", &rc);
    if (rc != 0 && rc != SYS_ENOENT) {
        bhd_err_build(rsp, rc, "invalid uuids16_is_complete value");
        return 1;
    }

    rc = ble_json_arr_uint32(parent,
                             "uuids32", 
                             sizeof req->adv_fields.uuids32 /
                               sizeof req->adv_fields.uuids32[0],
                             0,
                             0xffff,
                             req->adv_fields.uuids32,
                             &num_elems);
    if (rc == 0) {
        req->adv_fields.num_uuids32 = num_elems;
    } else if (rc != SYS_ENOENT) {
        bhd_err_build(rsp, rc, "invalid uuids32");
        return 1;
    }

    req->adv_fields.uuids32_is_complete =
        bhd_json_bool(parent, "uuids32_is_complete", &rc);
    if (rc != 0 && rc != SYS_ENOENT) {
        bhd_err_build(rsp, rc, "invalid uuids32_is_complete value");
        return 1;
    }

    rc = ble_json_arr_uuid(parent,
                           "uuids128",
                           sizeof uuids / sizeof uuids[0],
                           uuids,
                           &num_elems);
    if (rc == 0) {
        for (i = 0; i < num_elems; i++) {
            memcpy(req->adv_fields.uuids128[i], uuids[i].u128.value, 16);
        }
        req->adv_fields.num_uuids128 = num_elems;
    } else if (rc != SYS_ENOENT) {
        bhd_err_build(rsp, rc, "invalid uuids128");
        return 1;
    }

    req->adv_fields.uuids128_is_complete =
        bhd_json_bool(parent, "uuids128_is_complete", &rc);
    if (rc != 0 && rc != SYS_ENOENT) {
        bhd_err_build(rsp, rc, "invalid uuids128_is_complete value");
        return 1;
    }

    s = bhd_json_string(parent, "name", &rc);
    if (rc == 0) {
        slen = strlen(s);
        if (slen > sizeof req->adv_fields.name) {
            bhd_err_build(rsp, SYS_ERANGE, "invalid name");
            return 1;
        }
        memcpy(req->adv_fields.name, s, slen);
        req->adv_fields.name_len = slen;
    } else if (rc != SYS_ENOENT) {
        bhd_err_build(rsp, rc, "invalid name");
        return 1;
    }

    req->adv_fields.name_is_complete =
        bhd_json_bool(parent, "name_is_complete", &rc);
    if (rc != 0 && rc != SYS_ENOENT) {
        return bhd_err_build(rsp, rc,
                            "invalid name_is_complete value");
    }

    req->adv_fields.tx_pwr_lvl =
        bhd_json_int_bounds(parent, "tx_pwr_lvl", INT8_MIN, INT8_MAX, &rc);
    if (rc == 0) {
        req->adv_fields.tx_pwr_lvl_is_present = 1;
    } else if (rc != SYS_ENOENT) {
        bhd_err_build(rsp, rc, "invalid tx_pwr_lvl");
        return 1;
    }

    req->adv_fields.slave_itvl_min =
        bhd_json_int_bounds(parent, "slave_itvl_min", 0, UINT16_MAX, &rc);
    switch (rc) {
    case SYS_ENOENT:
        break;

    default:
        bhd_err_build(rsp, rc, "invalid slave_itvl_min");
        return 1;

    case 0:
        req->adv_fields.slave_itvl_max =
            bhd_json_int_bounds(parent, "slave_itvl_max", 0, UINT16_MAX, &rc);
        if (rc != 0) {
            bhd_err_build(rsp, rc, "invalid slave_itvl_max");
            return 1;
        }

        req->adv_fields.slave_itvl_range_is_present = 1;
        break;
    }

    bhd_json_hex_string(parent,
                        "svc_data_uuid16",
                        sizeof req->adv_fields.svc_data_uuid16,
                        req->adv_fields.svc_data_uuid16,
                        &num_bytes,
                        &rc);
    if (rc == 0) {
        req->adv_fields.svc_data_uuid16_len = num_bytes;
    } else if (rc != SYS_ENOENT) {
        bhd_err_build(rsp, rc, "invalid svc_data_uuid16");
        return 1;
    }

    rc = ble_json_arr_addr(parent,
                           "public_tgt_addrs", 
                           sizeof req->adv_fields.public_tgt_addrs /
                             sizeof req->adv_fields.public_tgt_addrs[0],
                           (uint8_t *)req->adv_fields.public_tgt_addrs,
                           &num_elems);
    if (rc == 0) {
        req->adv_fields.num_public_tgt_addrs = num_elems;
    } else if (rc != SYS_ENOENT) {
        bhd_err_build(rsp, rc, "invalid public_target_addrs");
        return 1;
    }

    req->adv_fields.appearance =
        bhd_json_int_bounds(parent, "appearance", 0, UINT16_MAX, &rc);
    if (rc == 0) {
        req->adv_fields.appearance_is_present = 1;
    } else if (rc != SYS_ENOENT) {
        bhd_err_build(rsp, rc, "invalid appearance");
        return 1;
    }

    req->adv_fields.adv_itvl =
        bhd_json_int_bounds(parent, "adv_itvl", 0, UINT16_MAX, &rc);
    if (rc == 0) {
        req->adv_fields.adv_itvl_is_present = 1;
    } else if (rc != SYS_ENOENT) {
        bhd_err_build(rsp, rc, "invalid adv_itvl");
        return 1;
    }

    bhd_json_hex_string(parent,
                        "svc_data_uuid32",
                        sizeof req->adv_fields.svc_data_uuid32,
                        req->adv_fields.svc_data_uuid32,
                        &num_bytes,
                        &rc);
    if (rc == 0) {
        req->adv_fields.svc_data_uuid32_len = num_bytes;
    } else if (rc != SYS_ENOENT) {
        bhd_err_build(rsp, rc, "invalid svc_data_uuid32");
        return 1;
    }

    bhd_json_hex_string(parent,
                        "svc_data_uuid128",
                        sizeof req->adv_fields.svc_data_uuid128,
                        req->adv_fields.svc_data_uuid128,
                        &num_bytes,
                        &rc);
    if (rc == 0) {
        req->adv_fields.svc_data_uuid128_len = num_bytes;
    } else if (rc != SYS_ENOENT) {
        bhd_err_build(rsp, rc, "invalid svc_data_uuid128");
        return 1;
    }

    s = bhd_json_string(parent, "uri", &rc);
    if (rc == 0) {
        slen = strlen(s);
        if (slen > sizeof req->adv_fields.uri) {
            rc = SYS_ERANGE;
            bhd_err_build(rsp, rc, "invalid uri");
            return 1;
        }
        memcpy(req->adv_fields.uri, s, slen);
        req->adv_fields.uri_len = slen;
    } else if (rc != SYS_ENOENT) {
        bhd_err_build(rsp, rc, "invalid uri");
        return 1;
    }

    bhd_json_hex_string(parent,
                        "mfg_data",
                        sizeof req->adv_fields.mfg_data,
                        req->adv_fields.mfg_data,
                        &num_bytes,
                        &rc);
    if (rc == 0) {
        req->adv_fields.mfg_data_len = num_bytes;
    } else if (rc != SYS_ENOENT) {
        bhd_err_build(rsp, rc, "invalid mfg_data");
        return 1;
    }

    bhd_gap_adv_fields(req, rsp);

    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_clear_svcs_req_run(cJSON *parent,
                       struct bhd_req *req, struct bhd_rsp *rsp)
{
    bhd_gatts_clear_svcs(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_add_svcs_req_run(cJSON *parent,
                     struct bhd_req *req, struct bhd_rsp *rsp)
{
    cJSON *item;
    cJSON *arr;
    char *err_msg;
    int num_svcs;
    int rc;
    int i;

    arr = bhd_json_arr(parent, "services", &rc);
    if (rc != 0) {
        rsp->add_svcs.status = rc;
        goto done;
    }

    num_svcs = bhd_arr_len(arr);
    req->add_svcs.svcs = malloc_success(
        num_svcs * sizeof *req->add_svcs.svcs);

    i = 0;
    cJSON_ArrayForEach(item, arr) {
        rc = bhd_json_svc(item, req->add_svcs.svcs + i, &err_msg);
        if (rc != 0) {
            rsp->add_svcs.status = rc;
            goto done;
        }
        i++;
        req->add_svcs.num_svcs++;
    }

    bhd_gatts_add_svcs(req, rsp);

done:
    for (i = 0; i < req->add_svcs.num_svcs; i++) {
        bhd_destroy_svc(req->add_svcs.svcs + i);
    }
    free(req->add_svcs.svcs);

    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_commit_svcs_req_run(cJSON *parent,
                   struct bhd_req *req, struct bhd_rsp *rsp)
{
    bhd_gatts_commit_svcs(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_access_status_req_run(cJSON *parent,
                          struct bhd_req *req, struct bhd_rsp *rsp)
{
    uint8_t buf[BLE_ATT_ATTR_MAX_LEN];
    int data_len;
    int rc;

    req->access_status.att_status =
        bhd_json_int_bounds(parent, "att_status", 0, UINT8_MAX, &rc);
    if (rc != 0) {
        rsp->access_status.status = rc;
        return 1;
    }

    bhd_json_hex_string(parent, "data", sizeof buf, buf, &data_len, &rc);
    if (rc != 0 && rc != SYS_ENOENT) {
        rsp->access_status.status = rc;
        return 1;
    }
    req->notify.data = buf;
    req->notify.data_len = data_len;

    bhd_gatts_access_status(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_notify_req_run(cJSON *parent,
                   struct bhd_req *req, struct bhd_rsp *rsp)
{
    uint8_t buf[BLE_ATT_ATTR_MAX_LEN + 3];
    int rc;

    req->notify.conn_handle =
        bhd_json_int_bounds(parent, "conn_handle", 0, 0xffff, &rc);
    if (rc != 0) {
        goto err;
    }

    req->notify.attr_handle =
        bhd_json_int_bounds(parent, "attr_handle", 0, 0xffff, &rc);
    if (rc != 0) {
        goto err;
    }

    req->notify.data = buf;
    bhd_json_hex_string(parent, "data", sizeof buf, buf,
                        &req->notify.data_len, &rc);

    bhd_gattc_notify(req, rsp);
    return 1;

err:
    rsp->notify.status = rc;
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_find_chr_req_run(cJSON *parent,
                     struct bhd_req *req, struct bhd_rsp *rsp)
{
    int rc;

    bhd_json_uuid(parent, "svc_uuid", &req->find_chr.svc_uuid, &rc);
    if (rc != 0) {
        rsp->find_chr.status = rc;
        return 1;
    }

    bhd_json_uuid(parent, "chr_uuid", &req->find_chr.chr_uuid, &rc);
    if (rc != 0) {
        rsp->find_chr.status = rc;
        return 1;
    }

    bhd_gatts_find_chr(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
static int
bhd_sm_inject_io_req_run(cJSON *parent,
                         struct bhd_req *req, struct bhd_rsp *rsp)
{
    int len;
    int rc;

    req->sm_inject_io.conn_handle =
        bhd_json_int_bounds(parent, "conn_handle", 0, UINT16_MAX, &rc);
    if (rc != 0) {
        rsp->sm_inject_io.status = rc;
        return 1;
    }

    req->sm_inject_io.action =
        bhd_json_sm_passkey_action(parent, "action", &rc);
    if (rc != 0) {
        rsp->sm_inject_io.status = rc;
        return 1;
    }

    switch (req->sm_inject_io.action) {
    case BLE_SM_IOACT_OOB:
        bhd_json_hex_string(parent, "oob_data",
                            sizeof req->sm_inject_io.oob_data,
                            req->sm_inject_io.oob_data, &len, &rc);
        if (rc != 0) {
            rsp->sm_inject_io.status = rc;
            return 1;
        }
        if (len != sizeof req->sm_inject_io.oob_data) {
            rsp->sm_inject_io.status = BLE_HS_EINVAL;
            return 1;
        }
        break;

    case BLE_SM_IOACT_INPUT:
        /* Fall through. */
    case BLE_SM_IOACT_DISP:
        req->sm_inject_io.passkey =
            bhd_json_int_bounds(parent, "passkey", 0, UINT32_MAX, &rc);
        if (rc != 0) {
            rsp->sm_inject_io.status = rc;
            return 1;
        }
        break;

    case BLE_SM_IOACT_NUMCMP:
        req->sm_inject_io.numcmp_accept =
            bhd_json_bool(parent, "numcmp_accept", &rc);
        if (rc != 0) {
            rsp->sm_inject_io.status = rc;
            return 1;
        }
        break;

    default:
        rsp->sm_inject_io.status = BLE_HS_EINVAL;
        return 1;
    }

    bhd_sm_inject_io(req, rsp);
    return 1;
}

/**
 * @return                      1 if a response should be sent;
 *                              0 for no response.
 */
int
bhd_req_dec(const char *json, struct bhd_rsp *out_rsp)
{
    bhd_req_run_fn *req_cb;
    struct bhd_err_rsp err;
    struct bhd_req req;
    cJSON *root;
    int rc;

    req = (struct bhd_req){{0}};

    out_rsp->hdr.op = BHD_MSG_OP_RSP;

    root = cJSON_Parse(json);
    if (root == NULL) {
        bhd_err_build(out_rsp, SYS_ERANGE, "invalid json");
        rc = 1;
        goto done;
    }

    err = bhd_msg_hdr_dec(root, &req.hdr);
    if (err.status != 0) {
        BHD_LOG(DEBUG, "failed to decode BHD header; json=%s\n", json);
        out_rsp->hdr.type = BHD_MSG_TYPE_ERR;
        out_rsp->err = err;
        rc = 1;
        goto done;
    }
    out_rsp->hdr.seq = req.hdr.seq;

    if (req.hdr.op != BHD_MSG_OP_REQ) {
        bhd_err_build(out_rsp, SYS_ERANGE, "invalid op");
        rc = 1;
        goto done;
    }

    req_cb = bhd_req_dispatch_find(req.hdr.type);
    if (req_cb == NULL) {
        bhd_err_build(out_rsp, SYS_ERANGE, "invalid type");
        rc = 1;
        goto done;
    }

    out_rsp->hdr.type = req.hdr.type;

    rc = req_cb(root, &req, out_rsp);

done:
    cJSON_Delete(root);
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
bhd_disc_all_dscs_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->disc_all_dscs.status);
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

static int
bhd_scan_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->scan.status);
    return 0;
}

static int
bhd_scan_cancel_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->scan_cancel.status);
    return 0;
}

static int
bhd_set_preferred_mtu_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->set_preferred_mtu.status);
    return 0;
}

static int
bhd_security_initiate_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->security_initiate.status);
    return 0;
}

static int
bhd_conn_find_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->conn_find.status);

    if (rsp->conn_find.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        bhd_json_add_int(parent, "conn_handle", rsp->conn_find.conn_handle);
    }

    bhd_json_add_int(parent, "conn_itvl", rsp->conn_find.conn_itvl);
    bhd_json_add_int(parent, "conn_latency", rsp->conn_find.conn_latency);
    bhd_json_add_int(parent, "supervision_timeout",
                     rsp->conn_find.supervision_timeout);
    bhd_json_add_int(parent, "role", rsp->conn_find.role);
    bhd_json_add_int(parent, "master_clock_accuracy",
                     rsp->conn_find.master_clock_accuracy);

    if (rsp->conn_find.our_id_addr.type != BHD_ADDR_TYPE_NONE) {
        bhd_json_add_addr_type(parent, "own_id_addr_type",
                               rsp->conn_find.our_id_addr.type);
        bhd_json_add_addr(parent, "own_id_addr",
                          rsp->conn_find.our_id_addr.val);
    }

    if (rsp->conn_find.our_ota_addr.type != BHD_ADDR_TYPE_NONE) {
        bhd_json_add_addr_type(parent, "own_ota_addr_type",
                               rsp->conn_find.our_ota_addr.type);
        bhd_json_add_addr(parent, "own_ota_addr",
                          rsp->conn_find.our_ota_addr.val);
    }

    if (rsp->conn_find.peer_id_addr.type != BHD_ADDR_TYPE_NONE) {
        bhd_json_add_addr_type(parent, "peer_id_addr_type",
                               rsp->conn_find.peer_id_addr.type);
        bhd_json_add_addr(parent, "peer_id_addr",
                          rsp->conn_find.peer_id_addr.val);
    }

    if (rsp->conn_find.peer_ota_addr.type != BHD_ADDR_TYPE_NONE) {
        bhd_json_add_addr_type(parent, "peer_ota_addr_type",
                               rsp->conn_find.peer_ota_addr.type);
        bhd_json_add_addr(parent, "peer_ota_addr",
                          rsp->conn_find.peer_ota_addr.val);
    }

    bhd_json_add_bool(parent, "encrypted", rsp->conn_find.encrypted);
    bhd_json_add_bool(parent, "authenticated", rsp->conn_find.authenticated);
    bhd_json_add_bool(parent, "bonded", rsp->conn_find.bonded);
    bhd_json_add_int(parent, "key_size", rsp->conn_find.key_size);

    return 0;
}

static int
bhd_reset_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    return 0;
}

static int
bhd_adv_start_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->adv_start.status);
    return 0;
}

static int
bhd_adv_stop_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->adv_stop.status);
    return 0;
}

static int
bhd_adv_set_data_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->adv_set_data.status);
    return 0;
}

static int
bhd_adv_rsp_set_data_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->adv_rsp_set_data.status);
    return 0;
}

static int
bhd_adv_fields_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->adv_fields.status);

    if (rsp->adv_fields.status == 0) {
        bhd_json_add_bytes(parent, "data", rsp->adv_fields.data,
                           rsp->adv_fields.data_len);
    }

    return 0;
}

static int
bhd_clear_svcs_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->clear_svcs.status);
    return 0;
}

static int
bhd_add_svcs_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->add_svcs.status);
    return 0;
}

static int
bhd_commit_svcs_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    cJSON *svcs;
    cJSON *svc;
    int rc;
    int i;

    bhd_json_add_int(parent, "status", rsp->commit_svcs.status);

    svcs = cJSON_CreateArray();
    if (svcs == NULL) {
        rc = SYS_ENOMEM;
        goto done;
    }

    for (i = 0; i < rsp->commit_svcs.num_svcs; i++) {
        svc = bhd_json_create_commit_svc(rsp->commit_svcs.svcs + i);
        if (svc == NULL) {
            rc = SYS_ENOMEM;
            goto done;
        }

        cJSON_AddItemToArray(svcs, svc);
    }

    cJSON_AddItemToObject(parent, "services", svcs);

    rc = 0;

done:
    for (i = 0; i < rsp->commit_svcs.num_svcs; i++) {
        free(rsp->commit_svcs.svcs[i].chrs);
    }
    free(rsp->commit_svcs.svcs);

    return rc;
}

static int
bhd_access_status_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->access_status.status);
    return 0;
}

static int
bhd_notify_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->notify.status);
    return 0;
}

static int
bhd_find_chr_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->find_chr.status);
    bhd_json_add_int(parent, "def_handle", rsp->find_chr.def_handle);
    bhd_json_add_int(parent, "val_handle", rsp->find_chr.val_handle);
    return 0;
}

static int
bhd_sm_inject_io_rsp_enc(cJSON *parent, const struct bhd_rsp *rsp)
{
    bhd_json_add_int(parent, "status", rsp->sm_inject_io.status);
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

    return 0;
}

static int
bhd_conn_cancel_evt_enc(cJSON *parent, const struct bhd_evt *evt)
{
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
bhd_disc_dsc_evt_enc(cJSON *parent, const struct bhd_evt *evt)
{
    char uuid_str[BLE_UUID_STR_LEN];
    cJSON *dsc;

    bhd_json_add_int(parent, "conn_handle", evt->disc_dsc.conn_handle);
    bhd_json_add_int(parent, "status", evt->disc_dsc.status);
    bhd_json_add_int(parent, "chr_def_handle", evt->disc_dsc.chr_def_handle);

    if (evt->disc_dsc.status == 0) {
        dsc = cJSON_CreateObject();
        if (dsc == NULL) {
            return SYS_ENOMEM;
        }

        cJSON_AddItemToObject(parent, "descriptor", dsc);

        bhd_json_add_int(dsc, "handle", evt->disc_dsc.dsc.handle);
        cJSON_AddStringToObject(dsc, "uuid",
                                ble_uuid_to_str(&evt->disc_dsc.dsc.uuid.u,
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
    char buf[BLE_ATT_ATTR_MAX_LEN * 5]; /* 0xXX[:] */

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

static int
bhd_scan_evt_enc(cJSON *parent, const struct bhd_evt *evt)
{
    char str[BLE_HS_ADV_MAX_FIELD_SZ + 1];

    bhd_json_add_adv_event_type(parent, "event_type", evt->scan.event_type);
    bhd_json_add_addr_type(parent, "addr_type", evt->scan.addr.type);
    bhd_json_add_addr(parent, "addr", evt->scan.addr.val);
    bhd_json_add_int(parent, "rssi", evt->scan.rssi);

    if (evt->scan.length_data > 0) {
        bhd_json_add_bytes(parent, "data", evt->scan.data,
                           evt->scan.length_data);
    }

    if (ble_addr_cmp(&evt->scan.direct_addr, BLE_ADDR_ANY) != 0) {
        bhd_json_add_addr_type(parent, "direct_addr_type",
                               evt->scan.direct_addr.type);
        bhd_json_add_addr(parent, "direct_addr", evt->scan.direct_addr.val);
    }

    if (evt->scan.data_flags != 0) {
        bhd_json_add_int(parent, "data_flags", evt->scan.data_flags);
    }

    if (evt->scan.data_num_uuids16 != 0) {
        bhd_json_add_gen_arr(parent,
                             "data_uuids16", 
                             evt->scan.data_uuids16,
                             evt->scan.data_num_uuids16,
                             cJSON_CreateNumber);
        bhd_json_add_bool(parent, "data_uuids16_is_complete",
                          evt->scan.data_uuids16_is_complete);
    }

    if (evt->scan.data_num_uuids32 != 0) {
        bhd_json_add_gen_arr(parent,
                             "data_uuids32", 
                             evt->scan.data_uuids32,
                             evt->scan.data_num_uuids32,
                             cJSON_CreateNumber);
        bhd_json_add_bool(parent, "data_uuids32_is_complete",
                          evt->scan.data_uuids32_is_complete);
    }

    if (evt->scan.data_num_uuids128 != 0) {
        bhd_json_add_gen_arr(parent,
                             "data_uuids128", 
                             evt->scan.data_uuids128,
                             evt->scan.data_num_uuids128,
                             bhd_json_create_uuid128_bytes);
        bhd_json_add_bool(parent, "data_uuids128_is_complete",
                          evt->scan.data_uuids128_is_complete);
    }

    if (evt->scan.data_name_len != 0) {
        memcpy(str, evt->scan.data_name, evt->scan.data_name_len);
        str[evt->scan.data_name_len] = '\0';
        cJSON_AddStringToObject(parent, "data_name", str);

        bhd_json_add_bool(parent, "data_name_is_complete",
                          evt->scan.data_name_is_complete);
    }

    if (evt->scan.data_tx_pwr_lvl_is_present) {
        bhd_json_add_int(parent, "data_tx_pwr_lvl", evt->scan.data_tx_pwr_lvl);
    }

    if (evt->scan.data_slave_itvl_range_is_present) {
        bhd_json_add_int(parent, "data_slave_itvl_min",
                         evt->scan.data_slave_itvl_min);
        bhd_json_add_int(parent, "data_slave_itvl_max",
                         evt->scan.data_slave_itvl_max);
    }

    if (evt->scan.data_svc_data_uuid16_len != 0) {
        bhd_json_add_bytes(parent, "data_svc_data_uuid16",
                           evt->scan.data_svc_data_uuid16,
                           evt->scan.data_svc_data_uuid16_len);
    }

    if (evt->scan.data_num_public_tgt_addrs != 0) {
        bhd_json_add_gen_arr(parent,
                             "data_public_tgt_addrs", 
                             evt->scan.data_public_tgt_addrs,
                             evt->scan.data_num_public_tgt_addrs,
                             bhd_json_create_addr);
    }

    if (evt->scan.data_appearance_is_present) {
        bhd_json_add_int(parent, "data_appearance", evt->scan.data_appearance);
    }

    if (evt->scan.data_adv_itvl_is_present) {
        bhd_json_add_int(parent, "data_adv_itvl", evt->scan.data_adv_itvl);
    }

    if (evt->scan.data_svc_data_uuid32_len != 0) {
        bhd_json_add_bytes(parent, "data_svc_data_uuid32",
                           evt->scan.data_svc_data_uuid32,
                           evt->scan.data_svc_data_uuid32_len);
    }

    if (evt->scan.data_svc_data_uuid128_len != 0) {
        bhd_json_add_bytes(parent, "data_svc_data_uuid128",
                           evt->scan.data_svc_data_uuid128,
                           evt->scan.data_svc_data_uuid128_len);
    }

    if (evt->scan.data_uri_len != 0) {
        memcpy(str, evt->scan.data_uri, evt->scan.data_uri_len);
        str[evt->scan.data_uri_len] = '\0';
        cJSON_AddStringToObject(parent, "data_uri", str);
    }

    if (evt->scan.data_mfg_data_len != 0) {
        bhd_json_add_bytes(parent, "data_mfg_data",
                           evt->scan.data_mfg_data,
                           evt->scan.data_mfg_data_len);
    }

    return 0;
}

static int
bhd_scan_tmo_evt_enc(cJSON *parent, const struct bhd_evt *evt)
{
    return 0;
}

static int
bhd_adv_complete_evt_enc(cJSON *parent, const struct bhd_evt *evt)
{
    bhd_json_add_int(parent, "reason", evt->adv_complete.reason);
    return 0;
}

static int
bhd_enc_change_evt_enc(cJSON *parent, const struct bhd_evt *evt)
{
    bhd_json_add_int(parent, "conn_handle", evt->enc_change.conn_handle);
    bhd_json_add_int(parent, "status", evt->enc_change.status);
    return 0;
}

static int
bhd_reset_evt_enc(cJSON *parent, const struct bhd_evt *evt)
{
    bhd_json_add_int(parent, "reason", evt->reset.reason);
    return 0;
}

static int
bhd_access_evt_enc(cJSON *parent, const struct bhd_evt *evt)
{
    bhd_json_add_gatt_access_op(parent, "access_op", evt->access.access_op);
    bhd_json_add_int(parent, "conn_handle", evt->access.conn_handle);
    bhd_json_add_int(parent, "att_handle", evt->access.att_handle);
    bhd_json_add_bytes(parent, "data", evt->access.data, evt->access.data_len);
    return 0;
}

static int
bhd_passkey_evt_enc(cJSON *parent, const struct bhd_evt *evt)
{
    bhd_json_add_int(parent, "conn_handle", evt->passkey.conn_handle);
    bhd_json_add_sm_passkey_action(parent, "action", evt->passkey.action);

    if (evt->passkey.action == BLE_SM_IOACT_NUMCMP) {
        bhd_json_add_int(parent, "numcmp", evt->passkey.numcmp);
    }
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

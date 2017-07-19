#include <assert.h>
#include "defs/error.h"
#include "host/ble_hs.h"
#include "bhd_proto.h"
#include "bhd_gatts.h"
#include "bhd_util.h"

#define BHD_GATTS_MAX_SVCS          32
#define BHD_GATTS_MAX_CHRS          512
#define BHD_GATTS_MAX_DSCS          2048
#define BHD_GATTS_MAX_UUIDS         \
    (BHD_GATTS_MAX_SVCS + BHD_GATTS_MAX_CHRS + BHD_GATTS_MAX_DSCS)
#define BHD_GATTS_MAX_VAL_HANDLES   BHD_GATTS_MAX_CHRS

#define BHD_GATTS_ACCESS_STATUS_TIMEOUT (10 * OS_TICKS_PER_SEC)
#define BHD_GATTS_ACCESS_STATUS_NONE    UINT8_MAX

static struct ble_gatt_svc_def bhd_gatts_svcs[BHD_GATTS_MAX_SVCS];
static struct ble_gatt_chr_def bhd_gatts_chrs[BHD_GATTS_MAX_CHRS];
static struct ble_gatt_dsc_def bhd_gatts_dscs[BHD_GATTS_MAX_DSCS];
static ble_uuid_any_t bhd_gatts_uuids[BHD_GATTS_MAX_UUIDS];
static uint16_t bhd_gatts_val_handles[BHD_GATTS_MAX_VAL_HANDLES];

static int bhd_gatts_num_svcs;
static int bhd_gatts_num_chrs;
static int bhd_gatts_num_dscs;
static int bhd_gatts_num_uuids;
static int bhd_gatts_num_val_handles;

static struct os_sem bhd_gatts_access_sem;
static uint8_t bhd_gatts_access_att_status = BHD_GATTS_ACCESS_STATUS_NONE;

static void
bhd_gatts_cnt_resources(const struct bhd_add_svcs_req *req,
                        int *out_num_svcs,
                        int *out_num_chrs,
                        int *out_num_dscs,
                        int *out_num_uuids,
                        int *out_num_val_handles)
{
    const struct bhd_svc *svc;
    const struct bhd_chr *chr;
    int s;
    int c;

    *out_num_svcs = req->num_svcs + 1;
    *out_num_chrs = 0;
    *out_num_dscs = 0;
    *out_num_val_handles = 0;

    for (s = 0; s < req->num_svcs; s++) {
        svc = req->svcs + s;
        *out_num_chrs += svc->num_chrs;
        *out_num_val_handles += svc->num_chrs;

        for (c = 0; c < svc->num_chrs; c++) {
            chr = svc->chrs + c;
            *out_num_dscs += chr->num_dscs;

            if (chr->num_dscs > 0) {
                (*out_num_dscs)++;
            }
        }

        if (*out_num_chrs > 0) {
            (*out_num_chrs)++;
        }
    }

    *out_num_uuids = *out_num_svcs + *out_num_chrs + *out_num_dscs;
}

static int
bhd_gatts_verify_resource_cnt(const struct bhd_add_svcs_req *req)
{
    int num_svcs;
    int num_chrs;
    int num_dscs;
    int num_uuids;
    int num_val_handles;

    bhd_gatts_cnt_resources(req, &num_svcs, &num_chrs, &num_dscs, &num_uuids,
                            &num_val_handles);

    if (bhd_gatts_num_svcs + num_svcs > BHD_GATTS_MAX_SVCS) {
        return BLE_HS_ENOMEM;
    }

    if (bhd_gatts_num_chrs + num_chrs > BHD_GATTS_MAX_CHRS) {
        return BLE_HS_ENOMEM;
    }

    if (bhd_gatts_num_dscs + num_dscs > BHD_GATTS_MAX_DSCS) {
        return BLE_HS_ENOMEM;
    }

    if (bhd_gatts_num_uuids + num_uuids > BHD_GATTS_MAX_UUIDS) {
        return BLE_HS_ENOMEM;
    }

    if (bhd_gatts_num_val_handles + num_val_handles >
        BHD_GATTS_MAX_VAL_HANDLES) {

        return BLE_HS_ENOMEM;
    }

    return 0;
}

int
bhd_gatts_set_access_status(uint8_t att_status)
{
    os_sr_t sr;
    int rc;

    OS_ENTER_CRITICAL(sr);

    if (bhd_gatts_access_att_status == BHD_GATTS_ACCESS_STATUS_NONE) {
        bhd_gatts_access_att_status = att_status;
        rc = 0;
    } else {
        rc = SYS_ENOENT;
    }

    OS_EXIT_CRITICAL(sr);

    if (rc == 0) {
        rc = os_sem_release(&bhd_gatts_access_sem);
    }

    return rc;
}

static uint8_t
bhd_gatts_wait_for_access_status(void)
{
    uint8_t status;
    int rc;

    rc = os_sem_pend(&bhd_gatts_access_sem, BHD_GATTS_ACCESS_STATUS_TIMEOUT);
    if (rc != 0) {
        status = BLE_ATT_ERR_UNLIKELY;
    } else {
        status = bhd_gatts_access_att_status;
    }

    bhd_gatts_access_att_status = BHD_GATTS_ACCESS_STATUS_NONE;
    return status;
}

static int
bhd_gatts_access(uint16_t conn_handle, uint16_t attr_handle,
                 struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint8_t buf[BLE_ATT_ATTR_MAX_LEN + 3];
    uint16_t data_len;
    struct bhd_access_evt access_evt;
    bhd_seq_t seq;
    int rc;

    seq = (bhd_seq_t)(uintptr_t)arg;

    access_evt.access_op = ctxt->op;
    access_evt.conn_handle = conn_handle;
    access_evt.att_handle = attr_handle;

    if (ctxt->om != NULL) {
        rc = ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof buf, &data_len);
        if (rc != 0) {
            /* XXX: Log error. */
            return BLE_ATT_ERR_UNLIKELY;
        }

        access_evt.data = buf;
        access_evt.data_len = data_len;
    }

    rc = bhd_send_access_evt(seq, &access_evt);
    if (rc != 0) {
        /* XXX: Log error. */
        return BLE_ATT_ERR_UNLIKELY;
    }

    /* Wait for client to indicate status via an access-status request. */
    return bhd_gatts_wait_for_access_status();
}

static ble_uuid_any_t *
bhd_gatts_save_uuid(const ble_uuid_any_t *src)
{
    ble_uuid_any_t *dst;

    assert(bhd_gatts_num_uuids < BHD_GATTS_MAX_UUIDS);

    dst = bhd_gatts_uuids + bhd_gatts_num_uuids;
    *dst = *src;
    bhd_gatts_num_uuids++;

    return dst;
}

static void
bhd_gatts_save_dsc(const struct bhd_dsc *dsc)
{
    struct ble_gatt_dsc_def *dstd;
    ble_uuid_any_t *dstu;

    assert(bhd_gatts_num_dscs < BHD_GATTS_MAX_DSCS);

    dstd = bhd_gatts_dscs + bhd_gatts_num_dscs;
    dstu = bhd_gatts_save_uuid(&dsc->uuid);

    dstd->uuid = &dstu->u;
    dstd->att_flags = dsc->att_flags;
    dstd->min_key_size = dsc->min_key_size;
    dstd->access_cb = bhd_gatts_access;
    dstd->arg = bhd_seq_arg(bhd_next_evt_seq());

    bhd_gatts_num_dscs++;
}

static void
bhd_gatts_save_chr(const struct bhd_chr *chr)
{
    struct ble_gatt_chr_def *dstc;
    ble_uuid_any_t *dstu;
    int i;

    assert(bhd_gatts_num_chrs < BHD_GATTS_MAX_CHRS);

    dstc = bhd_gatts_chrs + bhd_gatts_num_chrs;
    dstu = bhd_gatts_save_uuid(&chr->uuid);

    dstc->uuid = &dstu->u;
    dstc->flags = chr->flags;
    dstc->min_key_size = chr->min_key_size;
    dstc->access_cb = bhd_gatts_access;
    dstc->arg = bhd_seq_arg(bhd_next_evt_seq());

    dstc->val_handle = bhd_gatts_val_handles + bhd_gatts_num_val_handles;
    bhd_gatts_num_val_handles++;

    if (chr->num_dscs > 0) {
        dstc->descriptors = bhd_gatts_dscs + bhd_gatts_num_dscs;

        for (i = 0; i < chr->num_dscs; i++) {
            bhd_gatts_save_dsc(chr->dscs + i);
        }

        /* Null terminator. */
        bhd_gatts_num_dscs++;
    }

    bhd_gatts_num_chrs++;
}

static void
bhd_gatts_save_svc(const struct bhd_svc *svc)
{
    struct ble_gatt_svc_def *dsts;
    ble_uuid_any_t *dstu;
    int i;

    assert(bhd_gatts_num_svcs < BHD_GATTS_MAX_SVCS);

    dsts = bhd_gatts_svcs + bhd_gatts_num_svcs;
    dstu = bhd_gatts_save_uuid(&svc->uuid);

    dsts->type = svc->type;
    dsts->uuid = &dstu->u;

    if (svc->num_chrs > 0) {
        dsts->characteristics = bhd_gatts_chrs + bhd_gatts_num_chrs;

        for (i = 0; i < svc->num_chrs; i++) {
            bhd_gatts_save_chr(svc->chrs + i);
        }

        /* Null terminator. */
        bhd_gatts_num_chrs++;
    }

    bhd_gatts_num_svcs++;
}

void
bhd_gatts_clear_svcs(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    ble_gatts_reset();
    
    bhd_gatts_num_svcs = 0;
    bhd_gatts_num_chrs = 0;
    bhd_gatts_num_dscs = 0;
    bhd_gatts_num_uuids = 0;
    bhd_gatts_num_val_handles = 0;
}

void
bhd_gatts_add_svcs(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    int rc;
    int i;

    rc = bhd_gatts_verify_resource_cnt(&req->add_svcs);
    if (rc != 0) {
        out_rsp->add_svcs.status = rc;
        return;
    }

    for (i = 0; i < req->add_svcs.num_svcs; i++) {
        bhd_gatts_save_svc(req->add_svcs.svcs + i);
    }
    /* Null terminator. */
    bhd_gatts_num_svcs++;
}

void
bhd_gatts_commit_svcs(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    const struct ble_gatt_svc_def *src_svc;
    const struct ble_gatt_chr_def *src_chr;
    struct bhd_commit_svc *dst_svc;
    struct bhd_commit_chr *dst_chr;
    struct bhd_commit_svc *svcs;
    int si;
    int rc;
    int i;

    svcs = NULL;

    if (bhd_gatts_num_svcs > 0) {
        rc = ble_gatts_count_cfg(bhd_gatts_svcs);
        if (rc != 0) {
            goto err;
        }

        rc = ble_gatts_add_svcs(bhd_gatts_svcs);
        if (rc != 0) {
            goto err;
        }
    }

    rc = ble_gatts_start();
    if (rc != 0) {
        goto err;
    }

    svcs = calloc(bhd_gatts_num_svcs - 1, sizeof *svcs);
    if (svcs == NULL) {
        goto err;
    }

    for (si = 0; si < bhd_gatts_num_svcs - 1; si++) {
        src_svc = bhd_gatts_svcs + si;
        dst_svc = svcs + si;

        ble_uuid_copy(&dst_svc->uuid, src_svc->uuid);

        /* Count the number of characteristics. */
        for (dst_svc->num_chrs = 0; ; dst_svc->num_chrs++) {
            if (src_svc->characteristics[dst_svc->num_chrs].uuid == NULL) {
                break;
            }
        }

        dst_svc->chrs = calloc(dst_svc->num_chrs, sizeof *dst_svc->chrs);
        if (dst_svc->chrs == NULL) {
            goto err;
        }

        for (i = 0; i < dst_svc->num_chrs; i++) {
            src_chr = src_svc->characteristics + i;
            dst_chr = dst_svc->chrs + i;

            ble_uuid_copy(&dst_chr->uuid, src_chr->uuid);
            dst_chr->val_handle = *src_chr->val_handle;
            dst_chr->def_handle = dst_chr->val_handle - 1;

            /* XXX: Descriptors. */
        }
    }

    out_rsp->commit_svcs.num_svcs = bhd_gatts_num_svcs - 1;
    out_rsp->commit_svcs.svcs = svcs;

    return;

err:
    if (svcs != NULL) {
        for (i = 0; i < bhd_gatts_num_svcs; i++) {
            free(svcs[i].chrs);
        }
        free(svcs);
    }

    out_rsp->commit_svcs.status = rc;
}

void
bhd_gatts_access_status(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    out_rsp->access_status.status =
        bhd_gatts_set_access_status(req->access_status.att_status);
}

void
bhd_gatts_init(void)
{
    int rc;

    rc = os_sem_init(&bhd_gatts_access_sem, 0);
    assert(rc == 0);
}

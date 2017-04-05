#include <assert.h>
#include <string.h>

#include "blehostd.h"
#include "bhd_proto.h"
#include "bhd_gattc.h"
#include "bhd_util.h"
#include "defs/error.h"
#include "nimble/ble.h"
#include "host/ble_hs.h"

struct bhd_gattc_disc_chr_arg {
    uint16_t seq;
    uint16_t svc_start_handle;
};

struct bhd_gattc_disc_dsc_arg {
    uint16_t seq;
    uint16_t chr_val_handle;
};

static int
bhd_gattc_disc_svc_cb(uint16_t conn_handle,
                      const struct ble_gatt_error *error,
                      const struct ble_gatt_svc *service,
                      void *arg)
{
    struct bhd_evt evt = {0};
    uintptr_t seq;

    seq = (uintptr_t)arg;

    evt.hdr.op = BHD_MSG_OP_EVT;
    evt.hdr.type = BHD_MSG_TYPE_DISC_SVC_EVT;
    evt.hdr.seq = seq;

    evt.disc_svc.conn_handle = conn_handle;
    evt.disc_svc.status = error->status;

    if (error->status == 0) {
        evt.disc_svc.svc = *service;
    }

    bhd_evt_send(&evt);

    return 0;
}

static int
bhd_gattc_disc_chr_cb(uint16_t conn_handle,
                      const struct ble_gatt_error *error,
                      const struct ble_gatt_chr *chr,
                      void *arg)
{
    struct bhd_gattc_disc_chr_arg *chr_arg;
    struct bhd_evt evt = {0};

    chr_arg = arg;

    evt.hdr.op = BHD_MSG_OP_EVT;
    evt.hdr.type = BHD_MSG_TYPE_DISC_CHR_EVT;
    evt.hdr.seq = chr_arg->seq;

    evt.disc_chr.conn_handle = conn_handle;
    evt.disc_chr.status = error->status;

    if (error->status == 0) {
        evt.disc_chr.chr = *chr;
    } else {
        free(chr_arg);
    }

    bhd_evt_send(&evt);

    return 0;
}

static int
bhd_gattc_write_cb(uint16_t conn_handle,
                   const struct ble_gatt_error *error,
                   struct ble_gatt_attr *attr,
                   void *arg)
{
    struct bhd_evt evt = {0};
    uintptr_t seq;

    seq = (uintptr_t)arg;

    evt.hdr.op = BHD_MSG_OP_EVT;
    evt.hdr.type = BHD_MSG_TYPE_WRITE_ACK_EVT;
    evt.hdr.seq = seq;

    evt.write_ack.conn_handle = conn_handle;
    evt.write_ack.attr_handle = attr->handle;
    evt.write_ack.status = error->status;

    bhd_evt_send(&evt);

    return 0;
}

static int
bhd_gatt_mtu_cb(uint16_t conn_handle,
                const struct ble_gatt_error *error,
                uint16_t mtu,
                void *arg)
{
    uintptr_t seq;

    seq = (uintptr_t)arg;
    bhd_send_mtu_changed(seq, conn_handle, error->status, mtu);

    return 0;
}

void
bhd_gattc_disc_all_svcs(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    int rc;

    rc = ble_gattc_disc_all_svcs(req->disc_all_svcs.conn_handle,
                                 bhd_gattc_disc_svc_cb,
                                 bhd_seq_arg(req->hdr.seq));
    out_rsp->disc_all_svcs.status = rc;
}

void
bhd_gattc_disc_svc_uuid(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    int rc;

    rc = ble_gattc_disc_svc_by_uuid(req->disc_svc_uuid.conn_handle,
                                    &req->disc_svc_uuid.svc_uuid.u,
                                    bhd_gattc_disc_svc_cb,
                                    bhd_seq_arg(req->hdr.seq));
    out_rsp->disc_svc_uuid.status = rc;
}

void
bhd_gattc_disc_all_chrs(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    struct bhd_gattc_disc_chr_arg *chr_arg;
    int rc;

    chr_arg = malloc_success(sizeof *chr_arg);

    chr_arg->seq = req->hdr.seq;
    chr_arg->svc_start_handle = req->disc_all_chrs.start_attr_handle;

    rc = ble_gattc_disc_all_chrs(req->disc_all_chrs.conn_handle,
                                 req->disc_all_chrs.start_attr_handle,
                                 req->disc_all_chrs.end_attr_handle,
                                 bhd_gattc_disc_chr_cb, chr_arg);
    if (rc != 0) {
        free(chr_arg);
    }

    out_rsp->disc_all_chrs.status = rc;
}

void
bhd_gattc_disc_chr_uuid(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    struct bhd_gattc_disc_chr_arg *chr_arg;
    int rc;

    chr_arg = malloc_success(sizeof *chr_arg);
    chr_arg->seq = req->hdr.seq;
    chr_arg->svc_start_handle = req->disc_chr_uuid.start_handle;

    rc = ble_gattc_disc_chrs_by_uuid(req->disc_chr_uuid.conn_handle,
                                     req->disc_chr_uuid.start_handle,
                                     req->disc_chr_uuid.end_handle,
                                     &req->disc_chr_uuid.chr_uuid.u,
                                     bhd_gattc_disc_chr_cb,
                                     chr_arg);
    if (rc != 0) {
        free(chr_arg);
    }
    out_rsp->disc_chr_uuid.status = rc;
}

void
bhd_gattc_write(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    struct os_mbuf *om;
    int rc;

    om = ble_hs_mbuf_from_flat(req->write.data, req->write.data_len);
    if (om == NULL) {
        bhd_err_build(out_rsp, SYS_ENOMEM, "no mbufs available");
        return;
    }

    rc = ble_gattc_write(req->write.conn_handle, req->write.attr_handle,
                         om, bhd_gattc_write_cb,
                         bhd_seq_arg(req->hdr.seq));
    out_rsp->write.status = rc;
}

void
bhd_gattc_write_no_rsp(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    struct os_mbuf *om;
    int rc;

    om = ble_hs_mbuf_from_flat(req->write.data, req->write.data_len);
    if (om == NULL) {
        bhd_err_build(out_rsp, SYS_ENOMEM, "no mbufs available");
        return;
    }

    rc = ble_gattc_write_no_rsp(req->write.conn_handle, req->write.attr_handle,
                                om);
    out_rsp->write.status = rc;
}

void
bhd_gattc_exchange_mtu(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    int rc;

    rc = ble_gattc_exchange_mtu(req->exchange_mtu.conn_handle,
                                bhd_gatt_mtu_cb,
                                bhd_seq_arg(req->hdr.seq));
    out_rsp->exchange_mtu.status = rc;
}

void
bhd_gattc_set_preferred_mtu(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    int rc;

    rc = ble_att_set_preferred_mtu(req->set_preferred_mtu.mtu);
    out_rsp->set_preferred_mtu.status = rc;
}

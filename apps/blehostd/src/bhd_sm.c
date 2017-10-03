#include <assert.h>
#include <string.h>
#include "blehostd.h"
#include "bhd_proto.h"
#include "host/ble_hs.h"

void
bhd_sm_inject_oob(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    struct ble_sm_io io;

    io.action = BLE_SM_IOACT_OOB,
    memcpy(io.oob, req->oob_sec_data.data, sizeof io.oob);

    out_rsp->oob_sec_data.status =
        ble_sm_inject_io(req->oob_sec_data.conn_handle, &io);
}

#include <assert.h>
#include <string.h>
#include "blehostd.h"
#include "bhd_proto.h"
#include "host/ble_hs.h"

void
bhd_sm_inject_io(const struct bhd_req *req, struct bhd_rsp *out_rsp)
{
    struct ble_sm_io io;

    io.action = req->sm_inject_io.action;
    switch (io.action) {
    case BLE_SM_IOACT_OOB:
        memcpy(io.oob, req->sm_inject_io.oob_data, sizeof io.oob);
        break;

    case BLE_SM_IOACT_INPUT:
        /* Fall through. */
    case BLE_SM_IOACT_DISP:
        io.passkey = req->sm_inject_io.passkey;
        break;

    case BLE_SM_IOACT_NUMCMP:
        io.numcmp_accept = req->sm_inject_io.numcmp_accept;
        break;

    default:
        out_rsp->sm_inject_io.status = BLE_HS_EINVAL;
        return;
    }

    out_rsp->sm_inject_io.status =
        ble_sm_inject_io(req->sm_inject_io.conn_handle, &io);
}

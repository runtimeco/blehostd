#include "bhd_proto.h"
#include "bhd_id.h"
#include "host/ble_hs.h"

void
bhd_id_gen_rand_addr(const struct bhd_req *req, struct bhd_rsp *rsp)
{
    ble_addr_t addr;
    int rc;

    rc = ble_hs_id_gen_rnd(req->gen_rand_addr.nrpa, &addr);
    if (rc == 0) {
        memcpy(rsp->gen_rand_addr.addr, addr.val, 6);
    }

    rsp->gen_rand_addr.status = rc;
}

void
bhd_id_set_rand_addr(const struct bhd_req *req, struct bhd_rsp *rsp)
{
    rsp->set_rand_addr.status = ble_hs_id_set_rnd(req->set_rand_addr.addr);
}

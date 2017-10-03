#ifndef H_BHD_SM_
#define H_BHD_SM_

struct bhd_req;
struct bhd_rsp;

void bhd_sm_inject_io(const struct bhd_req *req, struct bhd_rsp *out_rsp);

#endif

#ifndef H_BHD_ID_
#define H_BHD_ID_

struct bhd_req;
struct bhd_rsp;

void bhd_id_gen_rand_addr(const struct bhd_req *req, struct bhd_rsp *rsp);
void bhd_id_set_rand_addr(const struct bhd_req *req, struct bhd_rsp *rsp);

#endif

#ifndef H_BHD_GATTS_
#define H_BHD_GATTS_

struct bhd_req;
struct bhd_rsp;

void bhd_gatts_clear_svcs(const struct bhd_req *req, struct bhd_rsp *out_rsp);
void bhd_gatts_add_svcs(const struct bhd_req *req, struct bhd_rsp *out_rsp);
void bhd_gatts_commit_svcs(const struct bhd_req *req, struct bhd_rsp *out_rsp);
void bhd_gatts_access_status(const struct bhd_req *req,
                             struct bhd_rsp *out_rsp);
void bhd_gatts_find_chr(const struct bhd_req *req, struct bhd_rsp *rsp);
void bhd_gatts_init(void);

#endif

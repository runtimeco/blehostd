#ifndef H_BHD_GATTC_
#define H_BHD_GATTC_

void bhd_gattc_disc_all_svcs(const struct bhd_req *req,
                             struct bhd_rsp *out_rsp);
void bhd_gattc_disc_svc_uuid(const struct bhd_req *req,
                             struct bhd_rsp *out_rsp);
void bhd_gattc_disc_all_chrs(const struct bhd_req *req,
                             struct bhd_rsp *out_rsp);
void bhd_gattc_disc_chr_uuid(const struct bhd_req *req,
                             struct bhd_rsp *out_rsp);
void bhd_gattc_write(const struct bhd_req *req, struct bhd_rsp *out_rsp);
void bhd_gattc_write_no_rsp(const struct bhd_req *req,
                            struct bhd_rsp *out_rsp);
void bhd_gattc_exchange_mtu(const struct bhd_req *req,
                            struct bhd_rsp *out_rsp);

#endif

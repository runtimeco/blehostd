#ifndef H_BHD_GAP_
#define H_BHD_GAP_

int bhd_gap_send_connect_evt(uint16_t seq, uint16_t conn_handle, int status);
void bhd_gap_connect(const struct bhd_req *req, struct bhd_rsp *out_rsp);
void bhd_gap_terminate(const struct bhd_req *req, struct bhd_rsp *out_rsp);
void bhd_gap_conn_cancel(const struct bhd_req *req, struct bhd_rsp *out_rsp);

#endif

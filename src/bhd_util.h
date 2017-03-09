#ifndef H_BHD_UTIL_
#define H_BHD_UTIL_

void *malloc_success(size_t num_bytes);
uint32_t bhd_next_evt_seq(void);
int bhd_send_mtu_changed(uint32_t seq, uint16_t conn_handle, int status,
                         uint16_t mtu);
int bhd_send_sync_evt(uint32_t seq);

#endif

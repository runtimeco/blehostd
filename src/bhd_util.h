#ifndef H_BHD_UTIL_
#define H_BHD_UTIL_

void *malloc_success(size_t num_bytes);
uint32_t bhd_next_evt_seq(void);
int bhd_send_mtu_changed(uint32_t seq, uint16_t conn_handle, int status,
                         uint16_t mtu);
int bhd_send_sync_evt(uint32_t seq);

long long int bhd_json_int(const cJSON *parent, const char *name, int *rc);
long long int bhd_json_int_bounds(const cJSON *parent, const char *name,
                                  long long int minval, long long int maxval,
                                  int *rc);
int bhd_json_bool(const cJSON *parent, const char *name, int *rc);
char *bhd_json_string(const cJSON *parent, const char *name, int *rc);
int bhd_json_addr_type(const cJSON *parent, const char *name, int *rc);
uint8_t *bhd_json_hex_string(const cJSON *parent, const char *name,
                             int max_len, uint8_t *dst, int *out_dst_len,
                             int *rc);
uint8_t *bhd_json_addr(const cJSON *parent, const char *name, uint8_t *dst,
                       int *rc);
ble_uuid_t *
bhd_json_uuid(const cJSON *parent, const char *name, ble_uuid_any_t *dst,
              int *status);
void bhd_json_add_int(cJSON *parent, const char *name, int64_t val);
void bhd_json_add_bool(cJSON *parent, const char *name, int val);
void bhd_json_add_uuid(cJSON *parent, const char *name,
                       const ble_uuid_t *uuid);
int bhd_json_add_addr_type(cJSON *parent, const char *name, uint8_t addr_type);
int bhd_json_add_addr(cJSON *parent, const char *name, const uint8_t *addr);

#endif

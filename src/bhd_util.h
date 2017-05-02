#ifndef H_BHD_UTIL_
#define H_BHD_UTIL_

void *malloc_success(size_t num_bytes);

bhd_seq_t bhd_next_evt_seq(void);

int bhd_op_parse(const char *op_str);
const char *bhd_op_rev_parse(int op);
int bhd_type_parse(const char *type_str);
const char *bhd_type_rev_parse(int type);
int bhd_addr_type_parse(const char *addr_type_str);
const char *bhd_addr_type_rev_parse(int addr_type);
int bhd_scan_filter_policy_parse(const char *scan_filter_policy_str);
const char *bhd_scan_filter_policy_rev_parse(int scan_filter_policy);
int bhd_adv_event_type_parse(const char *adv_event_type_str);
const char *bhd_adv_event_type_rev_parse(int adv_event_type);

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
int bhd_json_scan_filter_policy(const cJSON *parent, const char *name,
                                int *rc);
uint8_t *bhd_json_hex_string(const cJSON *parent, const char *name,
                             int max_len, uint8_t *dst, int *out_dst_len,
                             int *rc);
uint8_t *bhd_json_addr(const cJSON *parent, const char *name, uint8_t *dst,
                       int *rc);
ble_uuid_t *
bhd_json_uuid(const cJSON *parent, const char *name, ble_uuid_any_t *dst,
              int *status);
cJSON *bhd_json_create_byte_string(const uint8_t *data, int len);
cJSON *bhd_json_create_uuid(const ble_uuid_t *uuid);
cJSON *bhd_json_create_uuid128_bytes(const uint8_t uuid128_bytes[16]);
void bhd_json_add_int(cJSON *parent, const char *name, int64_t val);
void bhd_json_add_bool(cJSON *parent, const char *name, int val);
void bhd_json_add_bytes(cJSON *parent, const char *name, const uint8_t *data,
                        int len);
void bhd_json_add_uuid(cJSON *parent, const char *name,
                       const ble_uuid_t *uuid);
cJSON *bhd_json_create_addr(const uint8_t *addr);
int bhd_json_add_addr_type(cJSON *parent, const char *name, uint8_t addr_type);
int bhd_json_add_adv_event_type(cJSON *parent, const char *name,
                                uint8_t adv_event_type);
int bhd_json_add_addr(cJSON *parent, const char *name, const uint8_t *addr);

char *bhd_mbuf_to_s(const struct os_mbuf *om, char *str, size_t maxlen);

/**
 * Populates a JSON array with values from the specified C array and inserts it
 * into a JSON object.
 */
#define bhd_json_add_gen_arr(parent, name, vals, num_vals, create_cb) do    \
{                                                                           \
    cJSON *bhaga_item;                                                      \
    cJSON *bhaga_arr;                                                       \
    int bhaga_i;                                                            \
                                                                            \
    bhaga_arr = cJSON_CreateArray();                                        \
                                                                            \
    for (bhaga_i = 0; bhaga_i < (num_vals); bhaga_i++) {                    \
        bhaga_item = (create_cb)((vals)[bhaga_i]);                          \
        cJSON_AddItemToArray(bhaga_arr, bhaga_item);                        \
    }                                                                       \
                                                                            \
    cJSON_AddItemToObject((parent), (name), bhaga_arr);                     \
} while(0)

#endif

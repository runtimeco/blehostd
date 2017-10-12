#ifndef H_BHD_UTIL_
#define H_BHD_UTIL_

#include "cjson/cJSON.h"
struct bhd_access_evt;
struct bhd_commit_dsc;
struct bhd_commit_chr;
struct bhd_commit_svc;

typedef int bhd_json_fn(const cJSON *item, int *rc, void *arg);

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
int bhd_adv_conn_mode_parse(const char *conn_mode_str);
const char *bhd_adv_conn_mode_rev_parse(int conn_mode);
int bhd_adv_disc_mode_parse(const char *disc_mode_str);
const char *bhd_adv_disc_mode_rev_parse(int disc_mode);
int bhd_adv_filter_policy_parse(const char *filter_policy_str);
const char *bhd_adv_filter_policy_rev_parse(int filter_policy);
int bhd_svc_type_parse(const char *svc_type_str);
const char *bhd_svc_type_rev_parse(int svc_type);
int bhd_gatt_access_op_parse(const char *gatt_access_op_str);
const char *bhd_gatt_access_op_rev_parse(int gatt_access_op);
int bhd_sm_passkey_action_parse(const char *sm_passkey_action_str);
const char *bhd_sm_passkey_action_rev_parse(int sm_passkey_action);

int bhd_send_mtu_changed(uint32_t seq, uint16_t conn_handle, int status,
                         uint16_t mtu);
int bhd_send_sync_evt(uint32_t seq);
int bhd_send_reset_evt(bhd_seq_t seq, int reason);
int bhd_send_access_evt(bhd_seq_t seq, const struct bhd_access_evt *access);

long long int bhd_process_json_int(const cJSON *item, int *rc);
long long int bhd_json_int(const cJSON *parent, const char *name, int *rc);
long long int bhd_process_json_int_bounds(const cJSON *item,
                                          long long int minval,
                                          long long int maxval,
                                          int *rc);
uint8_t *bhd_process_json_hex_string(const cJSON *item, int max_len,
                                     uint8_t *dst, int *out_dst_len, int *rc);
uint8_t * bhd_process_json_addr(const cJSON *item, uint8_t *dst, int *rc);
ble_uuid_t *bhd_process_json_uuid(const cJSON *item, ble_uuid_any_t *dst,
                                  int *status);
long long int bhd_json_int_bounds(const cJSON *parent, const char *name,
                                  long long int minval, long long int maxval,
                                  int *rc);
int bhd_json_bool(const cJSON *parent, const char *name, int *rc);
char *bhd_json_string(const cJSON *parent, const char *name, int *rc);
cJSON *bhd_json_arr(const cJSON *parent, const char *name, int *rc);
int ble_json_arr_llong(const cJSON *parent, const char *name, int max_elems,
                       long long int minval, long long int maxval,
                       long long int *out_arr, int *out_num_elems);
int ble_json_arr_uint16(const cJSON *parent, const char *name, int max_elems,
                        uint16_t minval, uint16_t maxval,
                        uint16_t *out_arr, int *out_num_elems);
int ble_json_arr_uint32(const cJSON *parent, const char *name, int max_elems,
                        uint32_t minval, uint32_t maxval,
                        uint32_t *out_arr, int *out_num_elems);
int ble_json_arr_hex_string(const cJSON *parent, const char *name, int elem_sz,
                            int max_elems, uint8_t *out_arr,
                            int *out_num_elems);
int ble_json_arr_addr(const cJSON *parent, const char *name,
                      int max_elems, uint8_t *out_arr, int *out_num_elems);
int ble_json_arr_uuid(const cJSON *parent, const char *name,
                      int max_elems, ble_uuid_any_t *out_arr,
                      int *out_num_elems);
int bhd_json_addr_type(const cJSON *parent, const char *name, int *rc);
int bhd_json_scan_filter_policy(const cJSON *parent, const char *name,
                                int *rc);
int bhd_json_adv_conn_mode(const cJSON *parent, const char *name, int *rc);
int bhd_json_adv_disc_mode(const cJSON *parent, const char *name, int *rc);
int bhd_json_adv_filter_policy(const cJSON *parent, const char *name, int *rc);
int bhd_json_sm_passkey_action(cJSON *parent, const char *name, int *rc);
uint8_t *bhd_json_hex_string(const cJSON *parent, const char *name,
                             int max_len, uint8_t *dst, int *out_dst_len,
                             int *rc);
uint8_t *bhd_json_addr(const cJSON *parent, const char *name, uint8_t *dst,
                       int *rc);
ble_uuid_t *
bhd_json_uuid(const cJSON *parent, const char *name, ble_uuid_any_t *dst,
              int *status);
int bhd_json_dsc(cJSON *parent, struct bhd_dsc *out_dsc, char **out_err);
int bhd_json_chr(cJSON *parent, struct bhd_chr *out_chr, char **out_err);
int bhd_json_svc(cJSON *parent, struct bhd_svc *out_svc, char **out_err);
void bhd_destroy_chr(struct bhd_chr *chr);
void bhd_destroy_svc(struct bhd_svc *svc);

cJSON *bhd_json_create_byte_string(const uint8_t *data, int len);
cJSON *bhd_json_create_uuid(const ble_uuid_t *uuid);
cJSON *bhd_json_create_uuid128_bytes(const uint8_t uuid128_bytes[16]);
void bhd_json_add_int(cJSON *parent, const char *name, intmax_t val);
void bhd_json_add_bool(cJSON *parent, const char *name, int val);
cJSON * bhd_json_add_object(cJSON *parent, const char *name);
void bhd_json_add_bytes(cJSON *parent, const char *name, const uint8_t *data,
                        int len);
void bhd_json_add_uuid(cJSON *parent, const char *name,
                       const ble_uuid_t *uuid);
cJSON *bhd_json_create_addr(const uint8_t *addr);
int bhd_json_add_addr_type(cJSON *parent, const char *name, uint8_t addr_type);
int bhd_json_add_adv_event_type(cJSON *parent, const char *name,
                                uint8_t adv_event_type);
int bhd_json_add_gatt_access_op(cJSON *parent, const char *name,
                                uint8_t gatt_access_op);
int bhd_json_add_sm_passkey_action(cJSON *parent, const char *name,
                                   uint8_t sm_passkey_action);
cJSON *bhd_json_create_commit_dsc(const struct bhd_commit_dsc *dsc);
cJSON *bhd_json_create_commit_chr(const struct bhd_commit_chr *chr);
cJSON *bhd_json_create_commit_svc(const struct bhd_commit_svc *svc);
int bhd_json_add_addr(cJSON *parent, const char *name, const uint8_t *addr);

char *bhd_mbuf_to_s(const struct os_mbuf *om, char *str, size_t maxlen);
int bhd_arr_len(const cJSON *arr);

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

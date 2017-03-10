#ifndef H_BLEHOSTD_
#define H_BLEHOSTD_

#include <stdio.h>
#include <inttypes.h>
#include "syscfg/syscfg.h"
#include "os/os_mbuf.h"
#include "json/json.h"
#include "cjson/cJSON.h"
#include "host/ble_uuid.h"
struct bhd_req;
struct bhd_rsp;
struct bhd_evt;
struct bhd_dev;
struct bhd_connect_req;
struct peer;

extern int bhd_debug_log;

struct bhd_kv_str_int {
    const char *key;
    int val;
};

const struct bhd_kv_str_int *
bhd_kv_str_int_find_entry(const struct bhd_kv_str_int *map, const char *key);
const struct bhd_kv_str_int *
bhd_kv_str_int_rev_find_entry(const struct bhd_kv_str_int *map, int val);
int
bhd_kv_str_int_find_dflt(const struct bhd_kv_str_int *map, const char *key,
                         int dflt);
int
bhd_kv_str_int_find(const struct bhd_kv_str_int *map, const char *key);
const char *bhd_kv_str_int_rev_find(const struct bhd_kv_str_int *map, int val);

int bhd_op_parse(const char *op_str);
const char *bhd_op_rev_parse(int op);
int bhd_type_parse(const char *type_str);
const char *bhd_type_rev_parse(int type);
int bhd_addr_type_parse(const char *addr_type_str);
const char *bhd_addr_type_rev_parse(int addr_type);

char *bhd_hex_str(char *dst, int max_dst_len, int *out_dst_len,
                  const uint8_t *src, int src_len);
char *bhd_addr_str(char *dst, const uint8_t *addr);
void *bhd_seq_arg(uint16_t seq);

int bhd_msg_send(cJSON *root);
int bhd_rsp_send(const struct bhd_rsp *rsp);
int bhd_evt_send(const struct bhd_evt *evt);
int blehostd_enqueue_rsp(const char *json_rsp);
int bhd_req_dec(const char *json, struct bhd_rsp *out_rsp);
int bhd_rsp_enc(const struct bhd_rsp *rsp, cJSON **out_root);

#define BHD_LOG(lvl, ...)                                   \
    do {                                                    \
        if (MYNEWT_VAL(LOG_LEVEL) <= LOG_LEVEL_ ## lvl) {   \
            dprintf(1, __VA_ARGS__);                        \
        }                                                   \
    } while (0)

#endif

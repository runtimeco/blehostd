#include <assert.h>
#include <string.h>
#include "blehostd.h"
#include "parse.h"
#include "bhd_proto.h"
#include "bhd_util.h"
#include "defs/error.h"
#include "nimble/ble.h"
#include "nimble/hci_common.h"
#include "host/ble_hs.h"

typedef int bhd_kv_parse_fn(const char *src);

static const struct bhd_kv_str_int bhd_op_map[] = {
    { "request",        BHD_MSG_OP_REQ },
    { "response",       BHD_MSG_OP_RSP },
    { "event",          BHD_MSG_OP_EVT },
    { 0 },
};

static const struct bhd_kv_str_int bhd_type_map[] = {
    { "error",              BHD_MSG_TYPE_ERR },
    { "sync",               BHD_MSG_TYPE_SYNC },
    { "connect",            BHD_MSG_TYPE_CONNECT },
    { "terminate",          BHD_MSG_TYPE_TERMINATE },
    { "disc_all_svcs",      BHD_MSG_TYPE_DISC_ALL_SVCS },
    { "disc_svc_uuid",      BHD_MSG_TYPE_DISC_SVC_UUID },
    { "disc_all_chrs",      BHD_MSG_TYPE_DISC_ALL_CHRS },
    { "disc_chr_uuid",      BHD_MSG_TYPE_DISC_CHR_UUID },
    { "write",              BHD_MSG_TYPE_WRITE },
    { "write_cmd",          BHD_MSG_TYPE_WRITE_CMD },
    { "exchange_mtu",       BHD_MSG_TYPE_EXCHANGE_MTU },
    { "gen_rand_addr",      BHD_MSG_TYPE_GEN_RAND_ADDR },
    { "set_rand_addr",      BHD_MSG_TYPE_SET_RAND_ADDR },
    { "conn_cancel",        BHD_MSG_TYPE_CONN_CANCEL },
    { "scan",               BHD_MSG_TYPE_SCAN },
    { "scan_cancel",        BHD_MSG_TYPE_SCAN_CANCEL },
    { "set_preferred_mtu",  BHD_MSG_TYPE_SET_PREFERRED_MTU },
    { "security_initiate",  BHD_MSG_TYPE_ENC_INITIATE },
    { "conn_find",          BHD_MSG_TYPE_CONN_FIND },
    { "reset",              BHD_MSG_TYPE_RESET },
    { "adv_start",          BHD_MSG_TYPE_ADV_START },
    { "adv_stop",           BHD_MSG_TYPE_ADV_STOP },
    { "adv_set_data",       BHD_MSG_TYPE_ADV_SET_DATA },
    { "adv_rsp_set_data",   BHD_MSG_TYPE_ADV_RSP_SET_DATA },
    { "adv_fields",         BHD_MSG_TYPE_ADV_FIELDS },
    { "clear_svcs",         BHD_MSG_TYPE_CLEAR_SVCS },
    { "add_svcs",           BHD_MSG_TYPE_ADD_SVCS },
    { "commit_svcs",        BHD_MSG_TYPE_COMMIT_SVCS },
    { "access_status",      BHD_MSG_TYPE_ACCESS_STATUS },
    { "notify",             BHD_MSG_TYPE_NOTIFY },
    { "find_chr",           BHD_MSG_TYPE_FIND_CHR },

    { "sync_evt",           BHD_MSG_TYPE_SYNC_EVT },
    { "connect_evt",        BHD_MSG_TYPE_CONNECT_EVT },
    { "disconnect_evt",     BHD_MSG_TYPE_DISCONNECT_EVT },
    { "disc_svc_evt",       BHD_MSG_TYPE_DISC_SVC_EVT },
    { "disc_chr_evt",       BHD_MSG_TYPE_DISC_CHR_EVT },
    { "write_ack_evt",      BHD_MSG_TYPE_WRITE_ACK_EVT },
    { "notify_rx_evt",      BHD_MSG_TYPE_NOTIFY_RX_EVT },
    { "mtu_change_evt",     BHD_MSG_TYPE_MTU_CHANGE_EVT },
    { "scan_evt",           BHD_MSG_TYPE_SCAN_EVT },
    { "scan_tmo_evt",       BHD_MSG_TYPE_SCAN_TMO_EVT },
    { "enc_change_evt",     BHD_MSG_TYPE_ENC_CHANGE_EVT },
    { "reset_evt",          BHD_MSG_TYPE_RESET_EVT },
    { "access_evt",         BHD_MSG_TYPE_ACCESS_EVT },

    { 0 },
};

static const struct bhd_kv_str_int bhd_addr_type_map[] = {
    { "public",         BLE_ADDR_PUBLIC },
    { "random",         BLE_ADDR_RANDOM },
    { "rpa_pub",        BLE_ADDR_PUBLIC_ID },
    { "rpa_rnd",        BLE_ADDR_RANDOM_ID },
    { 0 },
};

static const struct bhd_kv_str_int bhd_scan_filter_policy_map[] = {
    { "no_wl",          BLE_HCI_SCAN_FILT_NO_WL },
    { "use_wl",         BLE_HCI_SCAN_FILT_USE_WL },
    { "no_wl_inita",    BLE_HCI_SCAN_FILT_NO_WL_INITA },
    { "use_wl_inita",   BLE_HCI_SCAN_FILT_USE_WL_INITA },
    { 0 },
};

static const struct bhd_kv_str_int bhd_adv_event_type_map[] = {
    { "ind",            BLE_HCI_ADV_TYPE_ADV_IND },
    { "direct_ind_hd",  BLE_HCI_ADV_TYPE_ADV_DIRECT_IND_HD },
    { "scan_ind",       BLE_HCI_ADV_TYPE_ADV_SCAN_IND },
    { "nonconn_ind",    BLE_HCI_ADV_TYPE_ADV_NONCONN_IND },
    { "direct_ind_ld",  BLE_HCI_ADV_TYPE_ADV_DIRECT_IND_LD },
    { 0 },
};

static const struct bhd_kv_str_int bhd_adv_conn_mode_map[] = {
    { "non",            BLE_GAP_CONN_MODE_NON },
    { "dir",            BLE_GAP_CONN_MODE_DIR },
    { "und",            BLE_GAP_CONN_MODE_UND },
    { 0 },
};

static const struct bhd_kv_str_int bhd_adv_disc_mode_map[] = {
    { "non",            BLE_GAP_DISC_MODE_NON },
    { "ltd",            BLE_GAP_DISC_MODE_LTD },
    { "gen",            BLE_GAP_DISC_MODE_GEN },
    { 0 },
};

static const struct bhd_kv_str_int bhd_adv_filter_policy_map[] = {
    { "none",           BLE_HCI_ADV_FILT_NONE },
    { "scan",           BLE_HCI_ADV_FILT_SCAN },
    { "conn",           BLE_HCI_ADV_FILT_CONN },
    { "both",           BLE_HCI_ADV_FILT_BOTH },
    { 0 },
};

static const struct bhd_kv_str_int bhd_svc_type_map[] = {
    { "primary",        BLE_GATT_SVC_TYPE_PRIMARY },
    { "secondary",      BLE_GATT_SVC_TYPE_SECONDARY },
    { 0 },
};

static const struct bhd_kv_str_int bhd_gatt_access_op_map[] = {
    { "read_chr",       BLE_GATT_ACCESS_OP_READ_CHR },
    { "write_chr",      BLE_GATT_ACCESS_OP_WRITE_CHR },
    { "read_dsc",       BLE_GATT_ACCESS_OP_READ_DSC },
    { "write_dsc",      BLE_GATT_ACCESS_OP_WRITE_DSC },
};

const struct bhd_kv_str_int *
bhd_kv_str_int_find_entry(const struct bhd_kv_str_int *map, const char *key)
{
    const struct bhd_kv_str_int *cur;

    for (cur = map; cur->key != NULL; cur++) {
        if (strcmp(cur->key, key) == 0) {
            return cur;
        }
    }

    return NULL;
}

const struct bhd_kv_str_int *
bhd_kv_str_int_rev_find_entry(const struct bhd_kv_str_int *map, int val)
{
    const struct bhd_kv_str_int *cur;

    for (cur = map; cur->key != NULL; cur++) {
        if (cur->val == val) {
            return cur;
        }
    }

    return NULL;
}

int
bhd_kv_str_int_find_dflt(const struct bhd_kv_str_int *map, const char *key,
                         int dflt)
{
    const struct bhd_kv_str_int *match;

    match = bhd_kv_str_int_find_entry(map, key);
    if (match == NULL) {
        return dflt;
    } else {
        return match->val;
    }
}

int
bhd_kv_str_int_find(const struct bhd_kv_str_int *map, const char *key)
{
    return bhd_kv_str_int_find_dflt(map, key, -1);
}

const char *
bhd_kv_str_int_rev_find(const struct bhd_kv_str_int *map, int val)
{
    const struct bhd_kv_str_int *match;

    match = bhd_kv_str_int_rev_find_entry(map, val);
    if (match == NULL) {
        return NULL;
    } else {
        return match->key;
    }
}

int
bhd_op_parse(const char *op_str)
{
    return bhd_kv_str_int_find(bhd_op_map, op_str);
}

const char *
bhd_op_rev_parse(int op)
{
    return bhd_kv_str_int_rev_find(bhd_op_map, op);
}

int
bhd_type_parse(const char *type_str)
{
    return bhd_kv_str_int_find(bhd_type_map, type_str);
}

const char *
bhd_type_rev_parse(int type)
{
    return bhd_kv_str_int_rev_find(bhd_type_map, type);
}

int
bhd_addr_type_parse(const char *addr_type_str)
{
    return bhd_kv_str_int_find(bhd_addr_type_map, addr_type_str);
}

const char *
bhd_addr_type_rev_parse(int addr_type)
{
    return bhd_kv_str_int_rev_find(bhd_addr_type_map, addr_type);
}

int
bhd_scan_filter_policy_parse(const char *filter_policy_str)
{
    return bhd_kv_str_int_find(bhd_scan_filter_policy_map, filter_policy_str);
}

const char *
bhd_scan_filter_policy_rev_parse(int filter_policy)
{
    return bhd_kv_str_int_rev_find(bhd_scan_filter_policy_map, filter_policy);
}

int
bhd_adv_event_type_parse(const char *adv_event_type_str)
{
    return bhd_kv_str_int_find(bhd_adv_event_type_map, adv_event_type_str);
}

const char *
bhd_adv_event_type_rev_parse(int adv_event_type)
{
    return bhd_kv_str_int_rev_find(bhd_adv_event_type_map, adv_event_type);
}

int
bhd_adv_conn_mode_parse(const char *conn_mode_str)
{
    return bhd_kv_str_int_find(bhd_adv_conn_mode_map, conn_mode_str);
}

const char *
bhd_adv_conn_mode_rev_parse(int conn_mode)
{
    return bhd_kv_str_int_rev_find(bhd_adv_conn_mode_map, conn_mode);
}

int
bhd_adv_disc_mode_parse(const char *disc_mode_str)
{
    return bhd_kv_str_int_find(bhd_adv_disc_mode_map, disc_mode_str);
}

const char *
bhd_adv_disc_mode_rev_parse(int disc_mode)
{
    return bhd_kv_str_int_rev_find(bhd_adv_disc_mode_map, disc_mode);
}

int
bhd_adv_filter_policy_parse(const char *filter_policy_str)
{
    return bhd_kv_str_int_find(bhd_adv_filter_policy_map, filter_policy_str);
}

const char *
bhd_adv_filter_policy_rev_parse(int filter_policy)
{
    return bhd_kv_str_int_rev_find(bhd_adv_filter_policy_map, filter_policy);
}

int
bhd_svc_type_parse(const char *svc_type_str)
{
    return bhd_kv_str_int_find(bhd_svc_type_map, svc_type_str);
}

const char *
bhd_svc_type_rev_parse(int svc_type)
{
    return bhd_kv_str_int_rev_find(bhd_svc_type_map, svc_type);
}

int
bhd_gatt_access_op_parse(const char *gatt_access_op_str)
{
    return bhd_kv_str_int_find(bhd_gatt_access_op_map, gatt_access_op_str);
}

const char *
bhd_gatt_access_op_rev_parse(int gatt_access_op)
{
    return bhd_kv_str_int_rev_find(bhd_gatt_access_op_map, gatt_access_op);
}

long long int
bhd_process_json_int(const cJSON *item, int *rc)
{
    if (item->type != cJSON_Number) {
        *rc = SYS_ERANGE;
        return -1;
    }

    *rc = 0;
    return item->valueint;
}

long long int
bhd_process_json_int_bounds(const cJSON *item,
                            long long int minval,
                            long long int maxval,
                            int *rc)
{
    long long int val;

    val = bhd_process_json_int(item, rc);
    if (*rc != 0) {
        return 0;
    }

    if (val < minval || val > maxval) {
        *rc = SYS_ERANGE;
        return 0;
    }

    return val;
}

char *
bhd_process_json_string(const cJSON *item, int *rc)
{
    if (item->type != cJSON_String) {
        *rc = SYS_ERANGE;
        return NULL;
    }

    *rc = 0;
    return item->valuestring;
}

uint8_t *
bhd_process_json_hex_string(const cJSON *item, int max_len,
                            uint8_t *dst, int *out_dst_len, int *rc)
{
    char *valstr;

    valstr = bhd_process_json_string(item, rc);
    switch (*rc) {
    case 0:
        *rc = parse_arg_byte_stream(valstr, max_len, dst, out_dst_len);
        return dst;

    case SYS_ENOENT:
        *rc = SYS_ERANGE;
        return NULL;

    default:
        return NULL;
    }
}

uint8_t *
bhd_process_json_addr(const cJSON *item, uint8_t *dst, int *rc)
{
    char *valstr;

    valstr = bhd_process_json_string(item, rc);
    if (*rc != 0) {
        return NULL;
    }

    *rc = parse_arg_mac(valstr, dst);
    return dst;
}

long long int
bhd_json_int(const cJSON *parent, const char *name, int *rc)
{
    cJSON *item;

    item = cJSON_GetObjectItem(parent, name);
    if (item == NULL) {
        *rc = SYS_ENOENT;
        return -1;
    }

    return bhd_process_json_int(item, rc);
}

int
bhd_json_bool(const cJSON *parent, const char *name, int *rc)
{
    cJSON *item;

    item = cJSON_GetObjectItem(parent, name);
    if (item == NULL) {
        *rc = SYS_ENOENT;
        return -1;
    }

    switch (item->type) {
    case cJSON_False:
        *rc = 0;
        return 0;

    case cJSON_True:
        *rc = 0;
        return 1;

    default:
        *rc = SYS_ERANGE;
        return 0;
    }
}

long long int
bhd_json_int_bounds(const cJSON *parent, const char *name,
                    long long int minval, long long int maxval, int *rc)
{
    cJSON *item;

    item = cJSON_GetObjectItem(parent, name);
    if (item == NULL) {
        *rc = SYS_ENOENT;
        return 0;
    }

    return bhd_process_json_int_bounds(item, minval, maxval, rc);
}

char *
bhd_json_string(const cJSON *parent, const char *name, int *rc)
{
    cJSON *item;

    item = cJSON_GetObjectItem(parent, name);
    if (item == NULL) {
        *rc = SYS_ENOENT;
        return NULL;
    }

    return bhd_process_json_string(item, rc);
}

uint8_t *
bhd_json_hex_string(const cJSON *parent, const char *name, int max_len,
                    uint8_t *dst, int *out_dst_len, int *rc)
{
    cJSON *item;

    item = cJSON_GetObjectItem(parent, name);
    if (item == NULL) {
        *rc = SYS_ENOENT;
        return NULL;
    }

    return bhd_process_json_hex_string(item, max_len, dst, out_dst_len, rc);
}

cJSON *
bhd_json_arr(const cJSON *parent, const char *name, int *rc)
{
    cJSON *item;

    item = cJSON_GetObjectItem(parent, name);
    if (item == NULL) {
        *rc = SYS_ENOENT;
        return NULL;
    }
    if (item->type != cJSON_Array) {
        *rc = SYS_ERANGE;
        return NULL;
    }

    *rc = 0;
    return item;
}

int
bhd_json_arr_gen(const cJSON *parent, const char *name,
                 bhd_json_fn *cb, void *arg)
{
    cJSON *item;
    cJSON *arr;
    int proceed;
    int idx;
    int rc;

    arr = bhd_json_arr(parent, name, &rc);
    if (rc != 0) {
        return rc;
    }

    idx = 0;
    cJSON_ArrayForEach(item, arr) {
        proceed = cb(item, &rc, arg);
        if (!proceed) {
            return rc;
        }

        idx++;
    }

    return 0;
}

struct ble_json_arr_arg_llong {
    long long int *dst;
    long long int minval;
    long long int maxval;
    int max_elems;
    int num_elems;
};

static int
ble_json_arr_fn_llong(const cJSON *item, int *rc, void *arg)
{
    struct ble_json_arr_arg_llong *barg;
    long long int val;

    barg = arg;

    if (barg->num_elems >= barg->max_elems) {
        *rc = SYS_ERANGE;
        return 1;
    }

    val = bhd_process_json_int_bounds(item, barg->minval, barg->maxval, rc);
    if (*rc != 0) {
        return 1;
    }

    barg->dst[barg->num_elems] = val;
    barg->num_elems++;
    return 0;
}

int
ble_json_arr_llong(const cJSON *parent, const char *name, int max_elems,
                   long long int minval, long long int maxval,
                   long long int *out_arr, int *out_num_elems)
{
    struct ble_json_arr_arg_llong arg;
    int rc;

    memset(&arg, 0, sizeof arg);
    arg.dst = out_arr;
    arg.minval = minval;
    arg.maxval = maxval;
    arg.max_elems = max_elems;

    rc = bhd_json_arr_gen(parent, name, ble_json_arr_fn_llong, &arg);
    if (rc != 0) {
        return rc;
    }

    *out_num_elems = arg.num_elems;
    return 0;
}

int
ble_json_arr_uint16(const cJSON *parent, const char *name, int max_elems,
                    uint16_t minval, uint16_t maxval,
                    uint16_t *out_arr, int *out_num_elems)
{
    long long int *vals;
    int rc;
    int i;

    vals = malloc_success(max_elems * sizeof *vals);

    rc = ble_json_arr_llong(parent, name, max_elems, minval, maxval,
                            vals, out_num_elems);
    if (rc == 0) {
        for (i = 0; i < *out_num_elems; i++) {
            out_arr[i] = vals[i];
        }
    }

    free(vals);

    return rc;
}

int
ble_json_arr_uint32(const cJSON *parent, const char *name, int max_elems,
                    uint32_t minval, uint32_t maxval,
                    uint32_t *out_arr, int *out_num_elems)
{
    long long int *vals;
    int rc;
    int i;

    vals = malloc_success(max_elems * sizeof *vals);

    rc = ble_json_arr_llong(parent, name, max_elems, minval, maxval,
                            vals, out_num_elems);
    if (rc == 0) {
        for (i = 0; i < *out_num_elems; i++) {
            out_arr[i] = vals[i];
        }
    }

    free(vals);

    return rc;
}

struct ble_json_arr_arg_hex_string {
    uint8_t *dst;
    int elem_sz;
    int max_elems;
    int num_elems;
};

static int
ble_json_arr_fn_hex_string(const cJSON *item, int *rc, void *arg)
{
    struct ble_json_arr_arg_hex_string *barg;
    uint8_t *elem;
    int num_bytes;

    barg = arg;

    if (barg->num_elems >= barg->max_elems) {
        *rc = SYS_ERANGE;
        return 1;
    }

    elem = barg->dst + barg->num_elems * barg->elem_sz;
    bhd_process_json_hex_string(item, barg->elem_sz, elem, &num_bytes, rc);
    if (*rc != 0) {
        return 1;
    }
    if (num_bytes != barg->elem_sz) {
        *rc = SYS_ERANGE;
        return 1;
    }

    barg->num_elems++;

    return 0;
}

int
ble_json_arr_hex_string(const cJSON *parent, const char *name, int elem_sz,
                        int max_elems, uint8_t *out_arr, int *out_num_elems)
{
    struct ble_json_arr_arg_hex_string arg;
    int rc;

    memset(&arg, 0, sizeof arg);
    arg.dst = out_arr;
    arg.elem_sz = elem_sz;
    arg.max_elems = max_elems;

    rc = bhd_json_arr_gen(parent, name, ble_json_arr_fn_hex_string, &arg);
    if (rc != 0) {
        return rc;
    }

    *out_num_elems = arg.num_elems;
    return 0;
}

struct ble_json_arr_arg_addr {
    uint8_t *dst;
    int max_elems;
    int num_elems;
};

static int
ble_json_arr_fn_addr(const cJSON *item, int *rc, void *arg)
{
    struct ble_json_arr_arg_addr *barg;
    uint8_t *elem;

    barg = arg;

    if (barg->num_elems >= barg->max_elems) {
        *rc = SYS_ERANGE;
        return 1;
    }

    elem = barg->dst + barg->num_elems * 6;
    bhd_process_json_addr(item, elem, rc);
    if (*rc != 0) {
        return 1;
    }

    barg->num_elems++;

    return 0;
}

int
ble_json_arr_addr(const cJSON *parent, const char *name,
                  int max_elems, uint8_t *out_arr, int *out_num_elems)
{
    struct ble_json_arr_arg_addr arg;
    int rc;

    memset(&arg, 0, sizeof arg);
    arg.dst = out_arr;
    arg.max_elems = max_elems;

    rc = bhd_json_arr_gen(parent, name, ble_json_arr_fn_addr, &arg);
    if (rc != 0) {
        return rc;
    }

    *out_num_elems = arg.num_elems;
    return 0;
}

static int
bhd_json_kv(bhd_kv_parse_fn *parse_cb, 
            const cJSON *parent, const char *name, int *rc)
{
    char *valstr;
    int valnum;

    valstr = bhd_json_string(parent, name, rc);
    switch (*rc) {
    case 0:
        valnum = parse_cb(valstr);
        if (valnum == -1) {
            *rc = SYS_ERANGE;
        } else {
            *rc = 0;
        }
        return valnum;

    default:
        return -1;
    }
}

int
bhd_json_addr_type(const cJSON *parent, const char *name, int *rc)
{
    return bhd_json_kv(bhd_addr_type_parse, parent, name, rc);
}

int
bhd_json_scan_filter_policy(const cJSON *parent, const char *name, int *rc)
{
    return bhd_json_kv(bhd_scan_filter_policy_parse, parent, name, rc);
}

int
bhd_json_adv_conn_mode(const cJSON *parent, const char *name, int *rc)
{
    return bhd_json_kv(bhd_adv_conn_mode_parse, parent, name, rc);
}

int
bhd_json_adv_disc_mode(const cJSON *parent, const char *name, int *rc)
{
    return bhd_json_kv(bhd_adv_disc_mode_parse, parent, name, rc);
}

int
bhd_json_adv_filter_policy(const cJSON *parent, const char *name, int *rc)
{
    return bhd_json_kv(bhd_adv_filter_policy_parse, parent, name, rc);
}

int
bhd_json_svc_type(const cJSON *parent, const char *name, int *rc)
{
    return bhd_json_kv(bhd_svc_type_parse, parent, name, rc);
}

uint8_t *
bhd_json_addr(const cJSON *parent, const char *name, uint8_t *dst, int *rc)
{
    const cJSON *item;

    item = cJSON_GetObjectItem(parent, name);
    if (item == NULL) {
        *rc = SYS_ENOENT;
        return NULL;
    }

    return bhd_process_json_addr(item, dst, rc);
}

ble_uuid_t *
bhd_json_uuid(const cJSON *parent, const char *name, ble_uuid_any_t *dst,
              int *status)
{
    unsigned long ul;
    const char *srcptr;
    uint16_t u16;
    uint8_t *dstptr;
    char *valstr;
    char *endptr;
    char buf[3];
    int vallen;
    int rc;
    int i;

    /* First, try a 16-bit UUID. */
    u16 = bhd_json_int_bounds(parent, name, 1, 0xffff, &rc);
    if (rc == 0) {
        dst->u.type = BLE_UUID_TYPE_16;
        dst->u16.value = u16;
        *status = 0;
        return &dst->u;
    }

    /* Next try a 128-bit UUID. */
    /* 00001101-0000-1000-8000-00805f9b34fb */
    valstr = bhd_json_string(parent, name, status);
    if (*status != 0) {
        return NULL;
    }

    vallen = strlen(valstr);
    if (vallen < BLE_UUID_STR_LEN - 1) {
        *status = SYS_ERANGE;
        return NULL;
    }

    srcptr = valstr;
    dstptr = dst->u128.value + 15;
    i = 0;
    while (i < 36) {
        switch (i) {
        case 8:
        case 13:
        case 18:
        case 23:
            if (srcptr[i] != '-') {
                *status = SYS_ERANGE;
                return NULL;
            }
            i++;
            break;

        default:
            buf[0] = srcptr[i + 0];
            buf[1] = srcptr[i + 1];
            buf[2] = '\0';

            ul = strtoul(buf, &endptr, 16);
            if (*endptr != '\0') {
                *status = SYS_ERANGE;
                return NULL;
            }
            *dstptr = ul;

            dstptr--;
            i += 2;
            break;
        }
    }

    dst->u.type = BLE_UUID_TYPE_128;
    *status = 0;
    return &dst->u;
}

int
bhd_json_dsc(cJSON *parent, struct bhd_dsc *out_dsc, char **out_err)
{
    int rc;

    bhd_json_uuid(parent, "uuid", &out_dsc->uuid, &rc);
    if (rc != 0) {
        *out_err = "invalid descriptor UUID";
        return rc;
    }

    out_dsc->att_flags = bhd_json_int_bounds(parent, "att_flags", 0, UINT8_MAX,
                                             &rc);
    if (rc != 0) {
        *out_err = "invalid descriptor att_flags";
        return rc;
    }

    out_dsc->min_key_size = bhd_json_int_bounds(parent, "min_key_size", 0,
                                                UINT8_MAX, &rc);
    if (rc != 0) {
        *out_err = "invalid descriptor flags";
        return rc;
    }

    return 0;
}

int
bhd_json_chr(cJSON *parent, struct bhd_chr *out_chr, char **out_err)
{
    cJSON *item;
    cJSON *arr;
    int rc;

    bhd_json_uuid(parent, "uuid", &out_chr->uuid, &rc);
    if (rc != 0) {
        *out_err = "invalid characteristic UUID";
        return rc;
    }

    out_chr->flags = bhd_json_int_bounds(parent, "flags", 0, UINT16_MAX, &rc);
    if (rc != 0) {
        *out_err = "invalid characteristic flags";
        return rc;
    }

    out_chr->min_key_size = bhd_json_int_bounds(parent, "min_key_size", 0,
                                                UINT8_MAX, &rc);
    if (rc != 0) {
        *out_err = "invalid characteristic flags";
        return rc;
    }

    arr = bhd_json_arr(parent, "descriptors", &rc);
    switch (rc) {
    case 0:
        out_chr->num_dscs = bhd_arr_len(arr);
        out_chr->dscs = malloc_success(
            out_chr->num_dscs * sizeof *out_chr->dscs);

        cJSON_ArrayForEach(item, arr) {
            rc = bhd_json_dsc(item, out_chr->dscs + out_chr->num_dscs,
                              out_err);
            if (rc != 0) {
                return rc;
            }
        }
        break;

    case SYS_ENOENT:
        break;

    default:
        *out_err = "invalid descriptors";
        return rc;
    }

    return 0;
}

int
bhd_json_svc(cJSON *parent, struct bhd_svc *out_svc, char **out_err)
{
    cJSON *item;
    cJSON *arr;
    int rc;
    int ci;

    out_svc->type = bhd_json_svc_type(parent, "type", &rc);
    if (rc != 0) {
        *out_err = "invalid service type";
        return rc;
    }

    bhd_json_uuid(parent, "uuid", &out_svc->uuid, &rc);
    if (rc != 0) {
        *out_err = "invalid service UUID";
        return rc;
    }

    arr = bhd_json_arr(parent, "characteristics", &rc);
    switch (rc) {
    case 0:
        out_svc->num_chrs = bhd_arr_len(arr);
        out_svc->chrs = malloc_success(
            out_svc->num_chrs * sizeof *out_svc->chrs);

        ci = 0;
        cJSON_ArrayForEach(item, arr) {
            rc = bhd_json_chr(item, out_svc->chrs + ci, out_err);
            if (rc != 0) {
                return rc;
            }
            ci++;
        }
        break;

    case SYS_ENOENT:
        break;

    default:
        *out_err = "invalid characteristics";
        return rc;
    }

    return 0;
}

void
bhd_destroy_chr(struct bhd_chr *chr)
{
    free(chr->dscs);
}

void
bhd_destroy_svc(struct bhd_svc *svc)
{
    int i;

    for (i = 0; i < svc->num_chrs; i++) {
        bhd_destroy_chr(svc->chrs + i);
    }
}

cJSON *
bhd_json_create_byte_string(const uint8_t *data, int len)
{
    cJSON *item;
    char *buf;
    int max_len;

    assert(len >= 0);

    max_len = len * 5; /* 0xXX: */

    buf = malloc_success(max_len);

    bhd_hex_str(buf, max_len, NULL, data, len);
    item = cJSON_CreateString(buf);

    free(buf);
    return item;
}

cJSON *
bhd_json_add_object(cJSON *parent, const char *name)
{
    struct cJSON *item;

    item = cJSON_CreateObject();
    if (item == NULL) {
        return NULL;
    }
    cJSON_AddItemToObject(parent, name, item);
    return item;
}

void
bhd_json_add_bytes(cJSON *parent, const char *name, const uint8_t *data,
                   int len)
{
    cJSON *item;

    item = bhd_json_create_byte_string(data, len);
    cJSON_AddItemToObject(parent, name, item);
}

cJSON *
bhd_json_create_uuid(const ble_uuid_t *uuid)
{
    char valstr[BLE_UUID_STR_LEN];

    switch (uuid->type) {
    case BLE_UUID_TYPE_16:
        return cJSON_CreateNumber(BLE_UUID16(uuid)->value);
    case BLE_UUID_TYPE_32:
        return cJSON_CreateNumber(BLE_UUID32(uuid)->value);
    case BLE_UUID_TYPE_128:
        return cJSON_CreateString(ble_uuid_to_str(uuid, valstr));
    default:
        assert(0);
        return NULL;
    }
}

cJSON *
bhd_json_create_uuid128_bytes(const uint8_t uuid128_bytes[16])
{
    ble_uuid128_t uuid128;

    uuid128.u.type = BLE_UUID_TYPE_128;
    memcpy(uuid128.value, uuid128_bytes, sizeof uuid128.value);

    return bhd_json_create_uuid(&uuid128.u);
}

void
bhd_json_add_uuid(cJSON *parent, const char *name, const ble_uuid_t *uuid)
{
    cJSON *item;

    item = bhd_json_create_uuid(uuid);
    cJSON_AddItemToObject(parent, name, item);
}

cJSON *
bhd_json_create_addr(const uint8_t *addr)
{
    char valstr[18];

    return cJSON_CreateString(bhd_addr_str(valstr, addr));
}

int
bhd_json_add_addr(cJSON *parent, const char *name, const uint8_t *addr)
{
    cJSON *item;

    item = bhd_json_create_addr(addr);
    cJSON_AddItemToObject(parent, name, item);
    return 0;
}

/**
 * This function is necessary to prevent the manifestation of a gcc-5 bug on
 * MacOS (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66509).
 */
void
bhd_json_add_int(cJSON *parent, const char *name, intmax_t val)
{
    cJSON *item;

    item = cJSON_CreateNumber(val);
    cJSON_AddItemToObject(parent, name, item);
}

void
bhd_json_add_bool(cJSON *parent, const char *name, int val)
{
    cJSON_AddItemToObject(parent, name, cJSON_CreateBool(val));
}

int
bhd_json_add_addr_type(cJSON *parent, const char *name, uint8_t addr_type)
{
    const char *valstr;

    valstr = bhd_addr_type_rev_parse(addr_type);
    if (valstr == NULL) {
        return SYS_EINVAL;
    }

    cJSON_AddStringToObject(parent, name, valstr);
    return 0;
}

int
bhd_json_add_adv_event_type(cJSON *parent, const char *name,
                            uint8_t adv_event_type)
{
    const char *valstr;

    valstr = bhd_adv_event_type_rev_parse(adv_event_type);
    if (valstr == NULL) {
        return SYS_EINVAL;
    }

    cJSON_AddStringToObject(parent, name, valstr);
    return 0;
}

int
bhd_json_add_gatt_access_op(cJSON *parent, const char *name,
                            uint8_t gatt_access_op)
{
    const char *valstr;

    valstr = bhd_gatt_access_op_rev_parse(gatt_access_op);
    if (valstr == NULL) {
        return SYS_EINVAL;
    }

    cJSON_AddStringToObject(parent, name, valstr);
    return 0;
}

cJSON *
bhd_json_create_commit_dsc(const struct bhd_commit_dsc *dsc)
{
    cJSON *item;

    item = cJSON_CreateObject();
    bhd_json_add_uuid(item, "uuid", &dsc->uuid.u);
    bhd_json_add_int(item, "handle", dsc->handle);

    return item;
}

cJSON *
bhd_json_create_commit_chr(const struct bhd_commit_chr *chr)
{
    cJSON *item;
    cJSON *dscs;
    cJSON *dsc;
    int i;

    item = cJSON_CreateObject();
    bhd_json_add_uuid(item, "uuid", &chr->uuid.u);
    bhd_json_add_int(item, "def_handle", chr->def_handle);
    bhd_json_add_int(item, "val_handle", chr->val_handle);

    dscs = cJSON_CreateArray();
    for (i = 0; i < chr->num_dscs; i++) {
        dsc = bhd_json_create_commit_dsc(chr->dscs + i);
        cJSON_AddItemToArray(dscs, dsc);
    }
    cJSON_AddItemToObject(item, "descriptors", dscs);

    return item;
}

cJSON *
bhd_json_create_commit_svc(const struct bhd_commit_svc *svc)
{
    cJSON *item;
    cJSON *chrs;
    cJSON *chr;
    int i;

    item = cJSON_CreateObject();
    if (item == NULL) {
        goto err;
    }

    bhd_json_add_uuid(item, "uuid", &svc->uuid.u);
    bhd_json_add_int(item, "handle", svc->handle);

    chrs = cJSON_CreateArray();
    if (chrs == NULL) {
        goto err;
    }

    for (i = 0; i < svc->num_chrs; i++) {
        chr = bhd_json_create_commit_chr(svc->chrs + i);
        if (chr == NULL) {
            goto err;
        }

        cJSON_AddItemToArray(chrs, chr);
    }
    cJSON_AddItemToObject(item, "characteristics", chrs);

    return item;

err:
    cJSON_Delete(item);
    return NULL;
}

char *
bhd_hex_str(char *dst, int max_dst_len, int *out_dst_len, const uint8_t *src,
            int src_len)
{
    int rem_len;
    int off;
    int rc;
    int i;

    off = 0;
    rem_len = max_dst_len;

    for (i = 0; i < src_len; i++) {
        rc = snprintf(dst + off, rem_len, "%s0x%02x",
                      i > 0 ? ":" : "", src[i]);
        if (rc >= rem_len) {
            break;
        }
        off += rc;
        rem_len -= rc;
    }

    if (out_dst_len != NULL) {
        *out_dst_len = off;
    }

    return dst;
}

char *
bhd_addr_str(char *dst, const uint8_t *addr)
{
    sprintf(dst, "%02x:%02x:%02x:%02x:%02x:%02x",
            addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
    return dst;
}

void *
bhd_seq_arg(bhd_seq_t seq)
{
    uintptr_t uiptr;
    void *v;

    uiptr = seq;
    v = (void *)uiptr;
    return v;
}

void *
malloc_success(size_t num_bytes)
{
    void *v;

    v = malloc(num_bytes);
    assert(v != NULL && "malloc returned null");

    return v;
}

bhd_seq_t
bhd_next_evt_seq(void)
{
    static bhd_seq_t next_seq = BHD_SEQ_EVT_MIN;

    bhd_seq_t seq;
    os_sr_t sr;

    OS_ENTER_CRITICAL(sr);
    seq = next_seq++;
    if (next_seq > BHD_SEQ_MAX) {
        next_seq = BHD_SEQ_EVT_MIN;
    }
    OS_EXIT_CRITICAL(sr);

    return seq;
}

int
bhd_send_mtu_changed(bhd_seq_t seq, uint16_t conn_handle, int status,
                     uint16_t mtu)
{
    struct bhd_evt evt;
    int rc;

    memset(&evt, 0, sizeof(evt));
    evt.hdr.op = BHD_MSG_OP_EVT;
    evt.hdr.type = BHD_MSG_TYPE_MTU_CHANGE_EVT;
    evt.hdr.seq = seq;

    evt.mtu_change.conn_handle = conn_handle;
    evt.mtu_change.mtu = mtu;
    evt.mtu_change.status = status;

    rc = bhd_evt_send(&evt);
    return rc;
}

int
bhd_send_sync_evt(bhd_seq_t seq)
{
    struct bhd_evt evt;
    int rc;

    memset(&evt, 0, sizeof(evt));
    evt.hdr.op = BHD_MSG_OP_EVT;
    evt.hdr.type = BHD_MSG_TYPE_SYNC_EVT;
    evt.hdr.seq = seq;

    evt.sync.synced = ble_hs_synced();

    rc = bhd_evt_send(&evt);
    return rc;
}

int
bhd_send_reset_evt(bhd_seq_t seq, int reason)
{
    struct bhd_evt evt;
    int rc;

    memset(&evt, 0, sizeof(evt));
    evt.hdr.op = BHD_MSG_OP_EVT;
    evt.hdr.type = BHD_MSG_TYPE_RESET_EVT;
    evt.hdr.seq = seq;

    evt.reset.reason = reason;

    rc = bhd_evt_send(&evt);
    return rc;
}

int
bhd_send_access_evt(bhd_seq_t seq, const struct bhd_access_evt *access)
{
    struct bhd_evt evt;
    int rc;

    evt.hdr.op = BHD_MSG_OP_EVT;
    evt.hdr.type = BHD_MSG_TYPE_ACCESS_EVT;
    evt.hdr.seq = seq;

    evt.access = *access;

    rc = bhd_evt_send(&evt);
    return rc;
}

char *
bhd_mbuf_to_s(const struct os_mbuf *om, char *str, size_t maxlen)
{
    char *dst;
    uint8_t byte;
    size_t remlen;
    size_t moff;
    int rc;

    dst = str;
    remlen = maxlen;

    for (moff = 0; moff < OS_MBUF_PKTLEN(om); moff++) {
        if (moff != 0) {
            rc = snprintf(dst, remlen, ":");
            dst += rc;
            remlen -= rc;
        }
        
        rc = os_mbuf_copydata(om, moff, 1, &byte);
        if (rc != 0) {
            break;
        }

        rc = snprintf(dst, remlen, "0x%02x", byte);
        dst += rc;
        remlen -= rc;
    }

    return str;
}

int
bhd_arr_len(const cJSON *arr)
{
    const cJSON *child;
    int len;

    len = 0;
    for (child = arr->child; child != NULL; child = child->next) {
        len++;
    }

    return len;
}

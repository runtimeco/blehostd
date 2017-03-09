#include <assert.h>
#include <string.h>
#include "blehostd.h"
#include "parse.h"
#include "bhd_proto.h"
#include "defs/error.h"
#include "nimble/ble.h"
#include "host/ble_hs.h"

static const struct bhd_kv_str_int bhd_op_map[] = {
    { "request",        BHD_MSG_OP_REQ },
    { "response",       BHD_MSG_OP_RSP },
    { "event",          BHD_MSG_OP_EVT },
    { 0 },
};

static const struct bhd_kv_str_int bhd_type_map[] = {
    { "error",          BHD_MSG_TYPE_ERR },
    { "sync",           BHD_MSG_TYPE_SYNC },
    { "connect",        BHD_MSG_TYPE_CONNECT },
    { "terminate",      BHD_MSG_TYPE_TERMINATE },
    { "disc_all_svcs",  BHD_MSG_TYPE_DISC_ALL_SVCS },
    { "disc_svc_uuid",  BHD_MSG_TYPE_DISC_SVC_UUID },
    { "disc_all_chrs",  BHD_MSG_TYPE_DISC_ALL_CHRS },
    { "disc_chr_uuid",  BHD_MSG_TYPE_DISC_CHR_UUID },
    { "write",          BHD_MSG_TYPE_WRITE },
    { "write_cmd",      BHD_MSG_TYPE_WRITE_CMD },
    { "exchange_mtu",   BHD_MSG_TYPE_EXCHANGE_MTU },

    { "sync_evt",       BHD_MSG_TYPE_SYNC_EVT },
    { "connect_evt",    BHD_MSG_TYPE_CONNECT_EVT },
    { "disconnect_evt", BHD_MSG_TYPE_DISCONNECT_EVT },
    { "disc_svc_evt",   BHD_MSG_TYPE_DISC_SVC_EVT },
    { "disc_chr_evt",   BHD_MSG_TYPE_DISC_CHR_EVT },
    { "write_ack_evt",  BHD_MSG_TYPE_WRITE_ACK_EVT },
    { "notify_rx_evt",  BHD_MSG_TYPE_NOTIFY_RX_EVT },
    { "mtu_change_evt", BHD_MSG_TYPE_MTU_CHANGE_EVT },

    { 0 },
};

static const struct bhd_kv_str_int bhd_addr_type_map[] = {
    { "public",         BLE_ADDR_PUBLIC },
    { "random",         BLE_ADDR_RANDOM },
    { "rpa_pub",        BLE_ADDR_PUBLIC_ID },
    { "rpa_rnd",        BLE_ADDR_RANDOM_ID },
    { 0 },
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

long long int
bhd_json_int(const cJSON *parent, const char *name, int *rc)
{
    cJSON *item;

    item = cJSON_GetObjectItem(parent, name);
    if (item == NULL) {
        *rc = SYS_ENOENT;
        return -1;
    }
    if (item->type != cJSON_Number) {
        *rc = SYS_ERANGE;
        return -1;
    }

    *rc = 0;
    return item->valueint;
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
    long long int val;

    val = bhd_json_int(parent, name, rc);
    if (*rc != 0) {
        return val;
    }

    if (val < minval || val > maxval) {
        *rc = SYS_ERANGE;
        return val;
    }

    return val;
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
    if (item->type != cJSON_String) {
        *rc = SYS_ERANGE;
        return NULL;
    }

    *rc = 0;
    return item->valuestring;
}

uint8_t *
bhd_json_hex_string(const cJSON *parent, const char *name, int max_len,
                    uint8_t *dst, int *out_dst_len, int *rc)
{
    char *valstr;

    valstr = bhd_json_string(parent, name, rc);
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

int
bhd_json_addr_type(const cJSON *parent, const char *name, int *rc)
{
    uint8_t addr_type;
    char *valstr;

    valstr = bhd_json_string(parent, name, rc);
    switch (*rc) {
    case 0:
        addr_type = bhd_addr_type_parse(valstr);
        if (addr_type == -1) {
            *rc = SYS_ERANGE;
        } else {
            *rc = 0;
        }
        return addr_type;

    case SYS_ENOENT:
        *rc = SYS_ERANGE;
        return -1;

    default:
        return -1;
    }
}

uint8_t *
bhd_json_addr(const cJSON *parent, const char *name, uint8_t *dst, int *rc)
{
    char *valstr;

    valstr = bhd_json_string(parent, name, rc);
    if (*rc != 0) {
        return NULL;
    }

    *rc = parse_arg_mac(valstr, dst);
    return dst;
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
    while (i < 37) {
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

void
bhd_json_add_uuid(cJSON *parent, const char *name, const ble_uuid_t *uuid)
{
    char valstr[BLE_UUID_STR_LEN];

    cJSON_AddStringToObject(parent, name, ble_uuid_to_str(uuid, valstr));
}

int
bhd_json_add_addr(cJSON *parent, const char *name, const uint8_t *addr)
{
    char valstr[18];

    cJSON_AddStringToObject(parent, name, bhd_addr_str(valstr, addr));
    return 0;
}

/**
 * This function is necessary to prevent the manifestation of a gcc-5 bug on
 * MacOS (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66509).
 */
void
bhd_json_add_int(cJSON *parent, const char *name, int64_t val)
{
    cJSON_AddItemToObject(parent, name, cJSON_CreateNumber(val));
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
            addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    return dst;
}

void *
bhd_seq_arg(uint16_t seq)
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
    assert(v != NULL);

    return v;
}

uint32_t
bhd_next_evt_seq(void)
{
    static uint32_t next_seq = BHD_EVT_SEQ_MIN;

    uint32_t seq;
    os_sr_t sr;

    OS_ENTER_CRITICAL(sr);
    seq = next_seq++;
    if (next_seq < BHD_EVT_SEQ_MIN) {
        next_seq = BHD_EVT_SEQ_MIN;
    }
    OS_EXIT_CRITICAL(sr);

    return seq;
}

int
bhd_send_mtu_changed(uint32_t seq, uint16_t conn_handle, int status,
                     uint16_t mtu)
{
    struct bhd_evt evt = {0};
    int rc;

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
bhd_send_sync_evt(uint32_t seq)
{
    struct bhd_evt evt = {0};
    int rc;

    evt.hdr.op = BHD_MSG_OP_EVT;
    evt.hdr.type = BHD_MSG_TYPE_SYNC_EVT;
    evt.hdr.seq = seq;

    evt.sync.synced = ble_hs_synced();

    rc = bhd_evt_send(&evt);
    return rc;
}

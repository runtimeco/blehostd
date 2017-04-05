#include <assert.h>
#include <string.h>
#include "blehostd.h"
#include "parse.h"
#include "bhd_proto.h"
#include "defs/error.h"
#include "nimble/ble.h"
#include "nimble/hci_common.h"
#include "host/ble_hs.h"

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
    { "conn_cancel",        BHD_MSG_TYPE_CONN_CANCEL },
    { "scan",               BHD_MSG_TYPE_SCAN },
    { "scan_cancel",        BHD_MSG_TYPE_SCAN_CANCEL },
    { "set_preferred_mtu",  BHD_MSG_TYPE_SET_PREFERRED_MTU },

    { "sync_evt",       BHD_MSG_TYPE_SYNC_EVT },
    { "connect_evt",    BHD_MSG_TYPE_CONNECT_EVT },
    { "disconnect_evt", BHD_MSG_TYPE_DISCONNECT_EVT },
    { "disc_svc_evt",   BHD_MSG_TYPE_DISC_SVC_EVT },
    { "disc_chr_evt",   BHD_MSG_TYPE_DISC_CHR_EVT },
    { "write_ack_evt",  BHD_MSG_TYPE_WRITE_ACK_EVT },
    { "notify_rx_evt",  BHD_MSG_TYPE_NOTIFY_RX_EVT },
    { "mtu_change_evt", BHD_MSG_TYPE_MTU_CHANGE_EVT },
    { "scan_evt",       BHD_MSG_TYPE_SCAN_EVT },

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

int
bhd_json_scan_filter_policy(const cJSON *parent, const char *name, int *rc)
{
    uint8_t filter_policy;
    char *valstr;

    valstr = bhd_json_string(parent, name, rc);
    switch (*rc) {
    case 0:
        filter_policy = bhd_scan_filter_policy_parse(valstr);
        if (filter_policy == -1) {
            *rc = SYS_ERANGE;
        } else {
            *rc = 0;
        }
        return filter_policy;

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

cJSON *
bhd_json_create_byte_string(const uint8_t *data, int len)
{
    cJSON *item;
    char *buf;
    int max_len;

    max_len = len * 5; /* 0xXX: */

    buf = malloc(max_len);
    assert(buf != NULL);

    bhd_hex_str(buf, max_len, NULL, data, len);
    item = cJSON_CreateString(buf);

    free(buf);
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
bhd_json_add_int(cJSON *parent, const char *name, int64_t val)
{
    cJSON_AddItemToObject(parent, name, cJSON_CreateNumber(val));
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
    static bhd_seq_t next_seq = BHD_SEQ_EVT_MIN;

    uint32_t seq;
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

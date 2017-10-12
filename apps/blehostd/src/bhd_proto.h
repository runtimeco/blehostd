#ifndef H_BHD_PROTO_
#define H_BHD_PROTO_

#include <inttypes.h>
#include "blehostd.h"
#include "host/ble_hs.h"

#define BHD_MSG_OP_REQ                      1
#define BHD_MSG_OP_RSP                      2
#define BHD_MSG_OP_EVT                      3
            
#define BHD_MSG_TYPE_ERR                    1
#define BHD_MSG_TYPE_SYNC                   2
#define BHD_MSG_TYPE_CONNECT                3
#define BHD_MSG_TYPE_TERMINATE              4
#define BHD_MSG_TYPE_DISC_ALL_SVCS          5
#define BHD_MSG_TYPE_DISC_SVC_UUID          6
#define BHD_MSG_TYPE_DISC_ALL_CHRS          7
#define BHD_MSG_TYPE_DISC_CHR_UUID          8
#define BHD_MSG_TYPE_DISC_ALL_DSCS          9
#define BHD_MSG_TYPE_WRITE                  10
#define BHD_MSG_TYPE_WRITE_CMD              11
#define BHD_MSG_TYPE_EXCHANGE_MTU           12
#define BHD_MSG_TYPE_GEN_RAND_ADDR          13
#define BHD_MSG_TYPE_SET_RAND_ADDR          14
#define BHD_MSG_TYPE_CONN_CANCEL            15
#define BHD_MSG_TYPE_SCAN                   16
#define BHD_MSG_TYPE_SCAN_CANCEL            17
#define BHD_MSG_TYPE_SET_PREFERRED_MTU      18
#define BHD_MSG_TYPE_ENC_INITIATE           19
#define BHD_MSG_TYPE_CONN_FIND              20
#define BHD_MSG_TYPE_RESET                  21
#define BHD_MSG_TYPE_ADV_START              22
#define BHD_MSG_TYPE_ADV_STOP               23
#define BHD_MSG_TYPE_ADV_SET_DATA           24
#define BHD_MSG_TYPE_ADV_RSP_SET_DATA       25
#define BHD_MSG_TYPE_ADV_FIELDS             26
#define BHD_MSG_TYPE_CLEAR_SVCS             27
#define BHD_MSG_TYPE_ADD_SVCS               28
#define BHD_MSG_TYPE_COMMIT_SVCS            29
#define BHD_MSG_TYPE_ACCESS_STATUS          30
#define BHD_MSG_TYPE_NOTIFY                 31
#define BHD_MSG_TYPE_FIND_CHR               32
#define BHD_MSG_TYPE_SM_INJECT_IO           33

#define BHD_MSG_TYPE_SYNC_EVT               2049
#define BHD_MSG_TYPE_CONNECT_EVT            2050
#define BHD_MSG_TYPE_CONN_CANCEL_EVT        2051
#define BHD_MSG_TYPE_DISCONNECT_EVT         2052
#define BHD_MSG_TYPE_DISC_SVC_EVT           2053
#define BHD_MSG_TYPE_DISC_CHR_EVT           2054
#define BHD_MSG_TYPE_DISC_DSC_EVT           2055
#define BHD_MSG_TYPE_WRITE_ACK_EVT          2056
#define BHD_MSG_TYPE_NOTIFY_RX_EVT          2057
#define BHD_MSG_TYPE_MTU_CHANGE_EVT         2058
#define BHD_MSG_TYPE_SCAN_EVT               2059
#define BHD_MSG_TYPE_SCAN_TMO_EVT           2060
#define BHD_MSG_TYPE_ADV_COMPLETE_EVT       2061
#define BHD_MSG_TYPE_ENC_CHANGE_EVT         2062
#define BHD_MSG_TYPE_RESET_EVT              2063
#define BHD_MSG_TYPE_ACCESS_EVT             2064
#define BHD_MSG_TYPE_PASSKEY_EVT            2065

#define BHD_ADDR_TYPE_NONE                  255

#define BHD_SEQ_MIN                         0
#define BHD_SEQ_EVT_MIN                     0xffffff00
#define BHD_SEQ_MAX                         0xfffffff0

#define BHD_REG_MAX_SVCS                    32
#define BHD_SVC_MAX_CHRS                    16
#define BHD_CHR_MAX_DSCS                    4

struct bhd_msg_hdr {
    int op;
    int type;
    bhd_seq_t seq;
};

struct bhd_connect_req {
    /* Mandatory. */
    int own_addr_type;
    ble_addr_t peer_addr;

    /* Optional. */
    int32_t duration_ms;
    uint16_t scan_itvl;
    uint16_t scan_window;
    uint16_t itvl_min;
    uint16_t itvl_max;
    uint16_t latency;
    uint16_t supervision_timeout;
    uint16_t min_ce_len;
    uint16_t max_ce_len;
};

struct bhd_terminate_req {
    uint16_t conn_handle;
    uint8_t hci_reason;
};

struct bhd_disc_all_svcs_req {
    uint16_t conn_handle;
};

struct bhd_disc_svc_uuid_req {
    uint16_t conn_handle;
    ble_uuid_any_t svc_uuid;
};

struct bhd_disc_all_chrs_req {
    uint16_t conn_handle;
    uint16_t start_attr_handle;
    uint16_t end_attr_handle;
};

struct bhd_disc_chr_uuid_req {
    uint16_t conn_handle;
    uint16_t start_handle;
    uint16_t end_handle;
    ble_uuid_any_t chr_uuid;
};

struct bhd_disc_all_dscs_req {
    uint16_t conn_handle;
    uint16_t start_attr_handle;
    uint16_t end_attr_handle;
};

struct bhd_write_req {
    uint16_t conn_handle;
    uint16_t attr_handle;
    uint8_t *data;
    int data_len;
};

struct bhd_exchange_mtu_req {
    uint16_t conn_handle;
};

struct bhd_gen_rand_addr_req {
    uint8_t nrpa;
};

struct bhd_set_rand_addr_req {
    uint8_t addr[6];
};

struct bhd_scan_req {
    uint8_t own_addr_type;
    int32_t duration_ms;
    uint16_t itvl;
    uint16_t window;
    uint8_t filter_policy;
    uint8_t limited:1;
    uint8_t passive:1;
    uint8_t filter_duplicates:1;
};

struct bhd_set_preferred_mtu_req {
    uint16_t mtu;
};

struct bhd_security_initiate_req {
    uint16_t conn_handle;
};

struct bhd_conn_find_req {
    uint16_t conn_handle;
};

struct bhd_adv_start_req {
    uint8_t own_addr_type;
    ble_addr_t peer_addr;
    int32_t duration_ms;
    uint8_t conn_mode;
    uint8_t disc_mode;
    uint16_t itvl_min;
    uint16_t itvl_max;
    uint8_t channel_map;
    uint8_t filter_policy;
    uint8_t high_duty_cycle:1;
};

struct bhd_adv_set_data_req {
    uint8_t data[BLE_HCI_MAX_ADV_DATA_LEN];
    int data_len;
};

struct bhd_adv_rsp_set_data_req {
    uint8_t data[BLE_HCI_MAX_ADV_DATA_LEN];
    int data_len;
};

struct bhd_adv_fields_req {
    /*** 0x01 - Flags. */
    uint8_t flags;

    /*** 0x02,0x03 - 16-bit service class UUIDs. */
    uint16_t uuids16[BLE_HS_ADV_MAX_FIELD_SZ / 2];
    uint8_t num_uuids16;
    unsigned uuids16_is_complete:1;

    /*** 0x04,0x05 - 32-bit service class UUIDs. */
    uint32_t uuids32[BLE_HS_ADV_MAX_FIELD_SZ / 4];
    uint8_t num_uuids32;
    unsigned uuids32_is_complete:1;

    /*** 0x06,0x07 - 128-bit service class UUIDs. */
    uint8_t uuids128[BLE_HS_ADV_MAX_FIELD_SZ / 16][16];
    uint8_t num_uuids128;
    unsigned uuids128_is_complete:1;

    /*** 0x08,0x09 - Local name. */
    uint8_t name[BLE_HS_ADV_MAX_FIELD_SZ];
    uint8_t name_len;
    unsigned name_is_complete:1;

    /*** 0x0a - Tx power level. */
    int8_t tx_pwr_lvl;
    unsigned tx_pwr_lvl_is_present:1;

    /*** 0x0d - Slave connection interval range. */
    uint16_t slave_itvl_min;
    uint16_t slave_itvl_max;
    unsigned slave_itvl_range_is_present;

    /*** 0x16 - Service data - 16-bit UUID. */
    uint8_t svc_data_uuid16[BLE_HS_ADV_MAX_FIELD_SZ];
    uint8_t svc_data_uuid16_len;

    /*** 0x17 - Public target address. */
    uint8_t public_tgt_addrs[BLE_HS_ADV_MAX_FIELD_SZ / 6][6];
    uint8_t num_public_tgt_addrs;

    /*** 0x19 - Appearance. */
    uint16_t appearance;
    unsigned appearance_is_present:1;

    /*** 0x1a - Advertising interval. */
    uint16_t adv_itvl;
    unsigned adv_itvl_is_present:1;

    /*** 0x20 - Service data - 32-bit UUID. */
    uint8_t svc_data_uuid32[BLE_HS_ADV_MAX_FIELD_SZ];
    uint8_t svc_data_uuid32_len;

    /*** 0x21 - Service data - 128-bit UUID. */
    uint8_t svc_data_uuid128[BLE_HS_ADV_MAX_FIELD_SZ];
    uint8_t svc_data_uuid128_len;

    /*** 0x24 - URI. */
    uint8_t uri[BLE_HS_ADV_MAX_FIELD_SZ];
    uint8_t uri_len;

    /*** 0xff - Manufacturer specific data. */
    uint8_t mfg_data[BLE_HS_ADV_MAX_FIELD_SZ];
    uint8_t mfg_data_len;
};

struct bhd_dsc {
    ble_uuid_any_t uuid;
    uint8_t att_flags;
    uint8_t min_key_size;
};

struct bhd_chr {
    ble_uuid_any_t uuid;
    ble_gatt_chr_flags flags;
    uint8_t min_key_size;
    struct bhd_dsc *dscs;
    int num_dscs;
};

struct bhd_svc {
    uint8_t type;
    ble_uuid_any_t uuid;
    struct bhd_chr *chrs;
    int num_chrs;

    /* XXX: Includes. */
};

struct bhd_add_svcs_req {
    struct bhd_svc *svcs;
    int num_svcs;
};

struct bhd_access_status_req {
    uint8_t att_status;
    uint8_t *data;
    int data_len;
};

struct bhd_notify_req {
    uint16_t conn_handle;
    uint16_t attr_handle;
    uint8_t *data;
    int data_len;
};

struct bhd_find_chr_req {
    ble_uuid_any_t svc_uuid;
    ble_uuid_any_t chr_uuid;
};

struct bhd_sm_inject_io_req {
    uint16_t conn_handle;
    uint8_t action;

    /* Only one of these is present, depending on the value of `action`. */
    uint8_t oob_data[16];   /* Out-of-band. */
    uint32_t passkey;       /* Input or display. */
    uint8_t numcmp_accept;  /* Numeric comparison. */
};

struct bhd_req {
    struct bhd_msg_hdr hdr;
    union {
        struct bhd_connect_req connect;
        struct bhd_terminate_req terminate;
        struct bhd_disc_all_svcs_req disc_all_svcs;
        struct bhd_disc_svc_uuid_req disc_svc_uuid;
        struct bhd_disc_all_chrs_req disc_all_chrs;
        struct bhd_disc_chr_uuid_req disc_chr_uuid;
        struct bhd_disc_all_dscs_req disc_all_dscs;
        struct bhd_write_req write;
        struct bhd_exchange_mtu_req exchange_mtu;
        struct bhd_gen_rand_addr_req gen_rand_addr;
        struct bhd_set_rand_addr_req set_rand_addr;
        struct bhd_scan_req scan;
        struct bhd_set_preferred_mtu_req set_preferred_mtu;
        struct bhd_security_initiate_req security_initiate;
        struct bhd_conn_find_req conn_find;
        struct bhd_adv_start_req adv_start;
        struct bhd_adv_set_data_req adv_set_data;
        struct bhd_adv_rsp_set_data_req adv_rsp_set_data;
        struct bhd_adv_fields_req adv_fields;
        struct bhd_add_svcs_req add_svcs;
        struct bhd_access_status_req access_status;
        struct bhd_notify_req notify;
        struct bhd_find_chr_req find_chr;
        struct bhd_sm_inject_io_req sm_inject_io;
    };
};

struct bhd_err_rsp {
    int status;
    const char *msg;
};

struct bhd_sync_rsp {
    int synced;
};

struct bhd_connect_rsp {
    int status;
};

struct bhd_terminate_rsp {
    int status;
};

struct bhd_disc_all_svcs_rsp {
    int status;
};

struct bhd_disc_svc_uuid_rsp {
    int status;
};

struct bhd_disc_all_chrs_rsp {
    int status;
};

struct bhd_disc_chr_uuid_rsp {
    int status;
};

struct bhd_disc_all_dscs_rsp {
    int status;
};

struct bhd_write_rsp {
    int status;
};

struct bhd_exchange_mtu_rsp {
    int status;
};

struct bhd_gen_rand_addr_rsp {
    int status;

    /* Only present if status is 0. */
    uint8_t addr[6];
};

struct bhd_set_rand_addr_rsp {
    int status;
};

struct bhd_conn_cancel_rsp {
    int status;
};

struct bhd_scan_rsp {
    int status;
};

struct bhd_scan_cancel_rsp {
    int status;
};

struct bhd_set_preferred_mtu_rsp {
    int status;
};

struct bhd_security_initiate_rsp {
    int status;
};

struct bhd_conn_find_rsp {
    int status;

    uint16_t conn_handle;
    uint16_t conn_itvl;
    uint16_t conn_latency;
    uint16_t supervision_timeout;
    uint8_t role;
    uint8_t master_clock_accuracy;

    ble_addr_t our_id_addr;
    ble_addr_t peer_id_addr;
    ble_addr_t our_ota_addr;
    ble_addr_t peer_ota_addr;

    uint8_t encrypted;
    uint8_t authenticated;
    uint8_t bonded;
    uint8_t key_size;
};

struct bhd_adv_start_rsp {
    int status;
};

struct bhd_adv_stop_rsp {
    int status;
};

struct bhd_adv_set_data_rsp {
    int status;
};

struct bhd_adv_rsp_set_data_rsp {
    int status;
};

struct bhd_adv_fields_rsp {
    int status;

    uint8_t data[BLE_HCI_MAX_ADV_DATA_LEN];
    int data_len;
};

struct bhd_clear_svcs_rsp {
    int status;
};

struct bhd_add_svcs_rsp {
    int status;
};

struct bhd_commit_dsc {
    ble_uuid_any_t uuid;
    uint16_t handle;
};

struct bhd_commit_chr {
    ble_uuid_any_t uuid;
    uint16_t def_handle;
    uint16_t val_handle;
    struct bhd_commit_dsc *dscs;
    int num_dscs;
};

struct bhd_commit_svc {
    ble_uuid_any_t uuid;
    uint16_t handle;
    struct bhd_commit_chr *chrs;
    int num_chrs;
};

struct bhd_commit_svcs_rsp {
    int status;
    struct bhd_commit_svc *svcs;
    int num_svcs;
};

struct bhd_access_status_rsp {
    int status;
};

struct bhd_notify_rsp {
    int status;
};

struct bhd_find_chr_rsp {
    int status;
    uint16_t def_handle;
    uint16_t val_handle;
};

struct bhd_sm_inject_io_rsp {
    int status;
};

struct bhd_rsp {
    struct bhd_msg_hdr hdr;
    union {
        struct bhd_err_rsp err;
        struct bhd_sync_rsp sync;
        struct bhd_connect_rsp connect;
        struct bhd_terminate_rsp terminate;
        struct bhd_disc_all_svcs_rsp disc_all_svcs;
        struct bhd_disc_svc_uuid_rsp disc_svc_uuid;
        struct bhd_disc_all_chrs_rsp disc_all_chrs;
        struct bhd_disc_chr_uuid_rsp disc_chr_uuid;
        struct bhd_disc_all_dscs_rsp disc_all_dscs;
        struct bhd_write_rsp write;
        struct bhd_exchange_mtu_rsp exchange_mtu;
        struct bhd_gen_rand_addr_rsp gen_rand_addr;
        struct bhd_set_rand_addr_rsp set_rand_addr;
        struct bhd_conn_cancel_rsp conn_cancel;
        struct bhd_scan_rsp scan;
        struct bhd_scan_cancel_rsp scan_cancel;
        struct bhd_set_preferred_mtu_rsp set_preferred_mtu;
        struct bhd_security_initiate_rsp security_initiate;
        struct bhd_conn_find_rsp conn_find;
        struct bhd_adv_start_rsp adv_start;
        struct bhd_adv_stop_rsp adv_stop;
        struct bhd_adv_set_data_rsp adv_set_data;
        struct bhd_adv_rsp_set_data_rsp adv_rsp_set_data;
        struct bhd_adv_fields_rsp adv_fields;
        struct bhd_clear_svcs_rsp clear_svcs;
        struct bhd_add_svcs_rsp add_svcs;
        struct bhd_commit_svcs_rsp commit_svcs;
        struct bhd_access_status_rsp access_status;
        struct bhd_notify_rsp notify;
        struct bhd_find_chr_rsp find_chr;
        struct bhd_sm_inject_io_rsp sm_inject_io;
    };
};

struct bhd_sync_evt {
    int synced;
};

struct bhd_connect_evt {
    int status;
    uint16_t conn_handle;
};

struct bhd_disconnect_evt {
    struct ble_gap_conn_desc desc;
    int reason;
};

struct bhd_disc_svc_evt {
    uint16_t conn_handle;
    int status;
    struct ble_gatt_svc svc;
};

struct bhd_disc_chr_evt {
    uint16_t conn_handle;
    int status;
    struct ble_gatt_chr chr;
};

struct bhd_disc_dsc_evt {
    uint16_t conn_handle;
    int status;
    uint16_t chr_def_handle;
    struct ble_gatt_dsc dsc;
};

struct bhd_write_ack_evt {
    uint16_t conn_handle;
    uint16_t attr_handle;
    int status;
};

struct bhd_notify_rx_evt {
    uint16_t conn_handle;
    uint16_t attr_handle;
    uint8_t indication:1;
    const uint8_t *data;
    int data_len;
};

struct bhd_mtu_change_evt {
    uint16_t conn_handle;
    uint16_t mtu;
    int status;
};

struct bhd_scan_evt {
    uint8_t event_type;
    uint8_t length_data;
    ble_addr_t addr;
    int8_t rssi;
    uint8_t data[BLE_HS_ADV_MAX_SZ];

    /** Only present for directed advertisements. */
    ble_addr_t direct_addr;

    /** Advertisement data fields. */
    /*** 0x01 - Flags. */
    uint8_t data_flags;

    /*** 0x02,0x03 - 16-bit service class UUIDs. */
    uint16_t data_uuids16[BLE_HS_ADV_MAX_FIELD_SZ / 2];
    uint8_t data_num_uuids16;
    unsigned data_uuids16_is_complete:1;

    /*** 0x04,0x05 - 32-bit service class UUIDs. */
    uint32_t data_uuids32[BLE_HS_ADV_MAX_FIELD_SZ / 4];
    uint8_t data_num_uuids32;
    unsigned data_uuids32_is_complete:1;

    /*** 0x06,0x07 - 128-bit service class UUIDs. */
    uint8_t data_uuids128[BLE_HS_ADV_MAX_FIELD_SZ / 16][16];
    uint8_t data_num_uuids128;
    unsigned data_uuids128_is_complete:1;

    /*** 0x08,0x09 - Local name. */
    uint8_t data_name[BLE_HS_ADV_MAX_FIELD_SZ];
    uint8_t data_name_len;
    unsigned data_name_is_complete:1;

    /*** 0x0a - Tx power level. */
    int8_t data_tx_pwr_lvl;
    unsigned data_tx_pwr_lvl_is_present:1;

    /*** 0x0d - Slave connection interval range. */
    uint16_t data_slave_itvl_min;
    uint16_t data_slave_itvl_max;
    unsigned data_slave_itvl_range_is_present;

    /*** 0x16 - Service data - 16-bit UUID. */
    uint8_t data_svc_data_uuid16[BLE_HS_ADV_MAX_FIELD_SZ];
    uint8_t data_svc_data_uuid16_len;

    /*** 0x17 - Public target address. */
    uint8_t data_public_tgt_addrs[BLE_HS_ADV_MAX_FIELD_SZ / 6][6];
    uint8_t data_num_public_tgt_addrs;

    /*** 0x19 - Appearance. */
    uint16_t data_appearance;
    unsigned data_appearance_is_present:1;

    /*** 0x1a - Advertising interval. */
    uint16_t data_adv_itvl;
    unsigned data_adv_itvl_is_present:1;

    /*** 0x20 - Service data - 32-bit UUID. */
    uint8_t data_svc_data_uuid32[BLE_HS_ADV_MAX_FIELD_SZ];
    uint8_t data_svc_data_uuid32_len;

    /*** 0x21 - Service data - 128-bit UUID. */
    uint8_t data_svc_data_uuid128[BLE_HS_ADV_MAX_FIELD_SZ];
    uint8_t data_svc_data_uuid128_len;

    /*** 0x24 - URI. */
    uint8_t data_uri[BLE_HS_ADV_MAX_FIELD_SZ];
    uint8_t data_uri_len;

    /*** 0xff - Manufacturer specific data. */
    uint8_t data_mfg_data[BLE_HS_ADV_MAX_FIELD_SZ];
    uint8_t data_mfg_data_len;
};

struct bhd_enc_change_evt {
    uint16_t conn_handle;
    int status;
};

struct bhd_reset_evt {
    int reason;
};

struct bhd_access_evt {
    uint8_t access_op;
    uint16_t conn_handle;
    uint16_t att_handle;
    uint8_t *data;
    int data_len;
};

struct bhd_adv_complete_evt {
    int reason;
};

struct bhd_passkey_evt {
    uint16_t conn_handle;
    uint8_t action;

    /* Optional. */
    uint32_t numcmp;
};

struct bhd_evt {
    struct bhd_msg_hdr hdr;
    union {
        struct bhd_sync_evt sync;
        struct bhd_connect_evt connect;
        struct bhd_disconnect_evt disconnect;
        struct bhd_disc_svc_evt disc_svc;
        struct bhd_disc_chr_evt disc_chr;
        struct bhd_disc_dsc_evt disc_dsc;
        struct bhd_write_ack_evt write_ack;
        struct bhd_notify_rx_evt notify_rx;
        struct bhd_mtu_change_evt mtu_change;
        struct bhd_scan_evt scan;
        struct bhd_enc_change_evt enc_change;
        struct bhd_reset_evt reset;
        struct bhd_access_evt access;
        struct bhd_adv_complete_evt adv_complete;
        struct bhd_passkey_evt passkey;
    };
};

int bhd_err_build(struct bhd_rsp *rsp, int status, const char *msg);

#endif

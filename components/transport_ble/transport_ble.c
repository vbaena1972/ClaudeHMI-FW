#include "transport_ble.h"
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_bt.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"

#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "cJSON.h"

static const char *TAG = "BLE";

// Manufacturer data (Axira)
static uint8_t mfg_data[4] = {0xA9, 0x11, 0x00, 0x01};

// Estado global
static bool s_enabled = false;
static bool s_stack_ready = false;
static bool s_adv_on = false;
static bool s_notify_enabled = false;
static bool s_mesh_enabled = false;

static uint16_t s_conn = BLE_HS_CONN_HANDLE_NONE;

static char s_dev_name[32] = "Axira-BLE";

static uint8_t s_rx_buf[256];

static ble_cmd_handler_t s_handler = NULL;

// Declarada en tu proyecto
extern void ui_statusbar_request_refresh(void);

// Forward declarations
static void ble_start_adv(void);
static int gap_event(struct ble_gap_event *ev, void *arg);
static void host_task(void *param);
static void ble_on_sync(void);
static void ble_on_reset(int reason);

/******************** GATT ********************/

#define AXIRA_SVC_UUID 0xF00D
#define AXIRA_RX_UUID 0xF101
#define AXIRA_TX_UUID 0xF102

static uint16_t s_tx_handle = 0;

static void process_rx_json(const char *buf, int len)
{
    if (!s_handler)
        return;

    cJSON *root = cJSON_ParseWithLength(buf, len);
    if (!root)
        return;

    s_handler(buf, len);
    cJSON_Delete(root);
}

static int rx_access(uint16_t conn, uint16_t attr_handle,
                     struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn;
    (void)attr_handle;
    (void)arg;

    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
        return BLE_ATT_ERR_UNLIKELY;

    uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
    if (om_len == 0 || om_len > sizeof(s_rx_buf))
        return BLE_ATT_ERR_UNLIKELY;

    uint16_t out_len = 0;
    if (ble_hs_mbuf_to_flat(ctxt->om, s_rx_buf, sizeof(s_rx_buf), &out_len) != 0)
        return BLE_ATT_ERR_UNLIKELY;

    process_rx_json((const char *)s_rx_buf, out_len);
    return 0;
}

static int tx_access(uint16_t conn, uint16_t attr_handle,
                     struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn;
    (void)attr_handle;
    (void)arg;
    (void)ctxt;
    return BLE_ATT_ERR_UNLIKELY;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(AXIRA_SVC_UUID),
     .characteristics = (struct ble_gatt_chr_def[]){
         {
             .uuid = BLE_UUID16_DECLARE(AXIRA_RX_UUID),
             .access_cb = rx_access,
             .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
         },
         {
             .uuid = BLE_UUID16_DECLARE(AXIRA_TX_UUID),
             .access_cb = tx_access,
             .val_handle = &s_tx_handle,
             .flags = BLE_GATT_CHR_F_NOTIFY,
         },
         {0}}},
    {0}};

/******************** GAP EVENTS ********************/

static int gap_event(struct ble_gap_event *ev, void *arg)
{
    (void)arg;

    switch (ev->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        if (ev->connect.status == 0)
        {
            s_conn = ev->connect.conn_handle;
            ESP_LOGI(TAG, "BLE connected");
            ui_statusbar_request_refresh();
        }
        else
        {
            ESP_LOGW(TAG, "BLE connect failed; status=%d", ev->connect.status);
            s_conn = BLE_HS_CONN_HANDLE_NONE;
            if (s_adv_on && !ble_gap_adv_active())
                ble_start_adv();
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "BLE disconnected; reason=%d", ev->disconnect.reason);
        s_conn = BLE_HS_CONN_HANDLE_NONE;
        s_notify_enabled = false;
        ui_statusbar_request_refresh();
        if (s_adv_on && !ble_gap_adv_active())
            ble_start_adv();
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        if (ev->subscribe.attr_handle == s_tx_handle)
            s_notify_enabled = ev->subscribe.cur_notify;
        return 0;

    default:
        return 0;
    }
}

/******************** Advertising ********************/

static void ble_start_adv(void)
{
    if (!s_stack_ready)
        return;
    if (ble_gap_adv_active())
        return;

    struct ble_hs_adv_fields f;
    memset(&f, 0, sizeof(f));

    f.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    f.name = (uint8_t *)s_dev_name;
    f.name_len = strlen(s_dev_name);
    f.name_is_complete = 1;

    f.mfg_data = mfg_data;
    f.mfg_data_len = sizeof(mfg_data);

    int rc = ble_gap_adv_set_fields(&f);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "ble_gap_adv_set_fields rc=%d", rc);
        return;
    }

    struct ble_gap_adv_params p;
    memset(&p, 0, sizeof(p));
    p.conn_mode = BLE_GAP_CONN_MODE_UND;
    p.disc_mode = BLE_GAP_DISC_MODE_GEN;
    p.itvl_min = 0x20;
    p.itvl_max = 0x40;

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                           &p, gap_event, NULL);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "ble_gap_adv_start rc=%d", rc);
        return;
    }

    ESP_LOGI(TAG, "Advertising started");
}

/******************** Host task & callbacks ********************/

static void host_task(void *param)
{
    (void)param;
    nimble_port_run(); // No retorna hasta nimble_port_stop()
    nimble_port_freertos_deinit();
    vTaskDelete(NULL);
}

static void ble_on_sync(void)
{
    s_stack_ready = true;

    uint8_t addr_val[6] = {0};
    uint8_t addr_type;
    int rc = ble_hs_id_infer_auto(0, &addr_type);
    if (rc == 0)
    {
        rc = ble_hs_id_copy_addr(addr_type, addr_val, NULL);
        if (rc == 0)
        {
            ESP_LOGI(TAG, "BLE addr: %02X:%02X:%02X:%02X:%02X:%02X",
                     addr_val[5], addr_val[4], addr_val[3],
                     addr_val[2], addr_val[1], addr_val[0]);
        }
    }

    ble_svc_gap_device_name_set(s_dev_name);

    if (s_adv_on)
        ble_start_adv();
}

static void ble_on_reset(int reason)
{
    ESP_LOGE(TAG, "BLE reset, reason=%d", reason);
}

/******************** INIT ********************/

esp_err_t transport_ble_init(bool enable)
{
    if (s_enabled)
        return ESP_OK;

    if (!enable)
    {
        s_enabled = false;
        return ESP_OK;
    }

    esp_err_t ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGW(TAG, "esp_bt_controller_mem_release: %s", esp_err_to_name(ret));
    }

    int rc = nimble_port_init();
    if (rc != 0)
    {
        ESP_LOGE(TAG, "nimble_port_init rc=%d", rc);
        return ESP_FAIL;
    }

    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.sync_cb = ble_on_sync;

    // Seguridad básica: bonding, sin MITM (Just Works)
    ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm = 0;
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svcs);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "ble_gatts_count_cfg rc=%d", rc);
        return ESP_FAIL;
    }

    rc = ble_gatts_add_svcs(gatt_svcs);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "ble_gatts_add_svcs rc=%d", rc);
        return ESP_FAIL;
    }

    nimble_port_freertos_init(host_task);

    s_enabled = true;
    s_adv_on = true; // empezamos anunciando por defecto
    ESP_LOGI(TAG, "NimBLE initialized OK");
    return ESP_OK;
}

/******************** API PÚBLICA ********************/

void transport_ble_set_cmd_handler(ble_cmd_handler_t h)
{
    s_handler = h;
}

esp_err_t transport_ble_set_enabled(bool on)
{
    if (on && !s_enabled)
    {
        return transport_ble_init(true);
    }

    if (!on && s_enabled)
    {
        s_enabled = false;
        if (s_stack_ready)
        {
            if (ble_gap_adv_active())
                ble_gap_adv_stop();
            if (s_conn != BLE_HS_CONN_HANDLE_NONE)
                ble_gap_terminate(s_conn, BLE_ERR_REM_USER_CONN_TERM);
        }
    }
    return ESP_OK;
}

esp_err_t transport_ble_set_name(const char *name)
{
    if (!name || !name[0])
        return ESP_ERR_INVALID_ARG;

    strncpy(s_dev_name, name, sizeof(s_dev_name));
    s_dev_name[sizeof(s_dev_name) - 1] = '\0';

    if (s_stack_ready)
    {
        ble_svc_gap_device_name_set(s_dev_name);
        if (s_adv_on)
        {
            if (ble_gap_adv_active())
                ble_gap_adv_stop();
            ble_start_adv();
        }
    }
    return ESP_OK;
}

esp_err_t transport_ble_set_passkey_mode(bool enable, uint32_t passkey)
{
    // Stub: no se implementa passkey real con NimBLE en esta versión.
    ESP_LOGW(TAG,
             "transport_ble_set_passkey_mode(enable=%d, passkey=%06u) "
             "-> modo passkey aún no implementado (stub).",
             (int)enable, (unsigned)(passkey % 1000000U));
    return ESP_OK;
}

esp_err_t transport_ble_set_tx_power(uint8_t level)
{
    // Sin API directa en NimBLE/IDF aquí -> stub.
    ESP_LOGW(TAG,
             "transport_ble_set_tx_power(%u) -> no cambia TX real en NimBLE (stub).",
             (unsigned)level);
    return ESP_OK;
}

esp_err_t transport_ble_start_adv(void)
{
    s_adv_on = true;
    if (s_stack_ready)
        ble_start_adv();
    return ESP_OK;
}

esp_err_t transport_ble_stop_adv(void)
{
    s_adv_on = false;
    if (ble_gap_adv_active())
        ble_gap_adv_stop();
    return ESP_OK;
}

bool transport_ble_is_enabled(void)
{
    return s_enabled;
}

bool transport_ble_is_advertising(void)
{
    return s_adv_on && ble_gap_adv_active();
}

esp_err_t transport_ble_set_mesh_enabled(bool on)
{
    s_mesh_enabled = on;
    // BLE Mesh real no implementado aquí.
    return ESP_OK;
}

bool transport_ble_is_mesh_enabled(void)
{
    return s_mesh_enabled;
}

bool transport_ble_is_connected(void)
{
    return s_conn != BLE_HS_CONN_HANDLE_NONE;
}

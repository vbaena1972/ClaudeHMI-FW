#include "ui_statusbar_controller.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "ui.h"               /* extern iconos de estado + tema/iconos */
#include "wifi_mgr.h"
#include "eth_mgr.h"
#include "transport_ble.h"
#include "ui_wifi_main_icon.h"
#include "esp_log.h"
#include <stdbool.h>

static const char *TAG = "ui_statusbar";

/* Visibilidad (A/B) según configuración */
static bool s_vis_wifi = false, s_vis_eth = false, s_vis_bt = false, s_vis_cloud = false;

static lv_timer_t *s_watch_timer = NULL;

/* estados previos para no repintar innecesariamente */
static bool s_prev_eth_link = false;
static bool s_prev_wifi_link = false;
static int  s_prev_wifi_rssi = 0;
static bool s_prev_ble_conn = false;
static bool s_prev_cloud_conn = false;

static inline void set_visible(lv_obj_t *o, bool on)
{
    if (!o) return;
    if (on) lv_obj_clear_flag(o, LV_OBJ_FLAG_HIDDEN);
    else    lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
}

static inline void set_glyph(lv_obj_t *o, const char *sym, uint32_t color)
{
    if (!o || !lv_obj_is_valid(o)) return;
    lv_label_set_text(o, sym);
    lv_obj_set_style_text_color(o, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void statusbar_timer_cb(lv_timer_t *t)
{
    (void)t;

    wifi_netinfo_t wi = {0};
    (void)wifi_mgr_get_netinfo(&wi);
    bool eth = eth_mgr_is_up();
    bool ble = transport_ble_is_connected();
    extern bool cloud_mgr_connected(void);
    bool cloud = cloud_mgr_connected();

    bool changed =
        (eth != s_prev_eth_link) ||
        (wi.connected != s_prev_wifi_link) ||
        (wi.rssi_dbm != s_prev_wifi_rssi) ||
        (ble != s_prev_ble_conn) ||
        (cloud != s_prev_cloud_conn);

    if (!changed && !s_vis_wifi && !s_vis_eth && !s_vis_bt && !s_vis_cloud)
        return;

    s_prev_eth_link = eth;
    s_prev_wifi_link = wi.connected;
    s_prev_wifi_rssi = wi.rssi_dbm;
    s_prev_ble_conn = ble;
    s_prev_cloud_conn = cloud;

    /* === BLUETOOTH === */
    set_visible(ui_bluetoothStatusMain, s_vis_bt);
    if (s_vis_bt)
        set_glyph(ui_bluetoothStatusMain, UI_SYM_BLUETOOTH, ble ? UI_C_BLUE : UI_C_TEXT_MUTED);

    /* === ETHERNET === */
    set_visible(ui_ethernetStatusMain, s_vis_eth);
    if (s_vis_eth)
        set_glyph(ui_ethernetStatusMain, UI_SYM_NETWORK, eth ? UI_C_OK : UI_C_TEXT_MUTED);

    /* === WIFI (prioridad Ethernet: si hay cable, wifi atenuado) === */
    set_visible(ui_wifiStatusMain, s_vis_wifi);
    if (s_vis_wifi)
    {
        if (eth || !wi.connected)
            set_glyph(ui_wifiStatusMain, UI_SYM_WIFI, UI_C_TEXT_MUTED);
        else
        {
            int bars = ui_wifi_rssi_to_bars(wi.rssi_dbm);
            uint32_t c = (bars >= 2) ? UI_C_OK : UI_C_WARN;
            set_glyph(ui_wifiStatusMain, UI_SYM_WIFI, c);
        }
    }

    /* === CLOUD === */
    if (ui_cloudStatusMain && lv_obj_is_valid(ui_cloudStatusMain))
    {
        set_visible(ui_cloudStatusMain, s_vis_cloud);
        if (s_vis_cloud)
            set_glyph(ui_cloudStatusMain, UI_SYM_CLOUD_CHECK, cloud ? UI_C_TEAL : UI_C_TEXT_MUTED);
    }

    if (ui_statusMainComm && lv_obj_is_valid(ui_statusMainComm))
        lv_obj_invalidate(ui_statusMainComm);
}

void ui_statusbar_request_refresh(void)
{
    if (s_watch_timer) lv_timer_ready(s_watch_timer);
}

void ui_statusbar_controller_init(void)
{
    if (!s_watch_timer)
        s_watch_timer = lv_timer_create(statusbar_timer_cb, 500, NULL);
    ESP_LOGI(TAG, "statusbar controller iniciado");
    ui_statusbar_request_refresh();
}

void ui_statusbar_controller_deinit(void)
{
    if (s_watch_timer)
    {
        lv_timer_del(s_watch_timer);
        s_watch_timer = NULL;
    }
    s_vis_wifi = s_vis_eth = s_vis_bt = s_vis_cloud = false;
}

void ui_statusbar_set_enabled(bool wifi, bool eth, bool bt, bool cloud)
{
    s_vis_wifi = wifi;
    s_vis_eth = eth;
    s_vis_bt = bt;
    s_vis_cloud = cloud;
    ui_statusbar_request_refresh();
}

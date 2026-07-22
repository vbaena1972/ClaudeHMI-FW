#include "lvgl.h"
#include "ui.h"                 /* extern ui_wifiStatusMain + tema/iconos */
#include "wifi_mgr.h"
#include "ui_statusbar_controller.h"

#define TAG "ui_wifi_icon"

/* En la nueva HMI el icono WiFi es un LABEL con glifo Tabler; el "nivel" de señal
 * se representa por color (verde=fuerte, ámbar=medio/conectando, gris=off/débil). */

typedef struct
{
    lv_obj_t *lbl;
    wifi_status_t last_st;
    int last_bars;
    wifi_status_t pending_st;
    int pending_rssi;
    bool dirty;
    lv_timer_t *timer;
} wifi_icon_ctx_t;

static wifi_icon_ctx_t s_ctx = {0};

int ui_wifi_rssi_to_bars(int rssi_dbm)
{
    if (rssi_dbm <= -80) return 1;
    if (rssi_dbm <= -67) return 2;
    if (rssi_dbm <= -55) return 3;
    return 4;
}

static void apply_icon(wifi_status_t st, int bars)
{
    if (!s_ctx.lbl || !lv_obj_is_valid(s_ctx.lbl)) return;

    uint32_t color = UI_C_TEXT_MUTED;
    switch (st)
    {
    case WIFI_STATUS_OFF:
    case WIFI_STATUS_DISCONNECTED:
        color = UI_C_TEXT_MUTED;
        bars = 0;
        break;
    case WIFI_STATUS_CONNECTING:
        color = UI_C_WARN;
        bars = 1;
        break;
    case WIFI_STATUS_CONNECTED:
    default:
        color = (bars >= 3) ? UI_C_OK : (bars == 2 ? UI_C_OK : UI_C_WARN);
        break;
    }

    if (st != s_ctx.last_st || bars != s_ctx.last_bars)
    {
        lv_label_set_text(s_ctx.lbl, UI_SYM_WIFI);
        lv_obj_set_style_text_color(s_ctx.lbl, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
        s_ctx.last_st = st;
        s_ctx.last_bars = bars;
    }
}

static void timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    if (!s_ctx.dirty) return;
    s_ctx.dirty = false;

    int bars = 0;
    if (s_ctx.pending_st == WIFI_STATUS_CONNECTED)
        bars = ui_wifi_rssi_to_bars(s_ctx.pending_rssi);

    apply_icon(s_ctx.pending_st, bars);
}

static void wifi_status_cb(wifi_status_t st, int rssi_dbm)
{
    s_ctx.pending_st = st;
    s_ctx.pending_rssi = rssi_dbm;
    s_ctx.dirty = true;
    ui_statusbar_request_refresh();
}

void ui_wifi_main_icon_init(lv_obj_t *img_obj)
{
    s_ctx.lbl = img_obj;
    s_ctx.last_st = (wifi_status_t)(-1);
    s_ctx.last_bars = -1;
    s_ctx.pending_st = WIFI_STATUS_OFF;
    s_ctx.pending_rssi = -127;
    s_ctx.dirty = true;

    if (!s_ctx.timer)
        s_ctx.timer = lv_timer_create(timer_cb, 500, NULL);

    wifi_mgr_set_status_cb(wifi_status_cb);

    if (s_ctx.timer)
        lv_timer_ready(s_ctx.timer);
}

void ui_wifi_main_icon_deinit(void)
{
    if (s_ctx.timer)
    {
        lv_timer_del(s_ctx.timer);
        s_ctx.timer = NULL;
    }
    s_ctx.lbl = NULL;
}

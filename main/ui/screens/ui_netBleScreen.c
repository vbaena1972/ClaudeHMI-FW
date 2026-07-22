#include "ui_netBleScreen.h"
#include "ui_form.h"
#include "ui_widgets.h"
#include "ui_theme.h"
#include "ui_nav.h"
#include "ui.h"
#include "storage.h"
#include <string.h>
#include <stdlib.h>

#ifdef ESP_PLATFORM
#include "transport_ble.h"
#endif

/* Configuración Bluetooth LE (formulario) + acceso a "Configurar por app" (QR). */

lv_obj_t *ui_netBleScreen = NULL;
static lv_obj_t *s_sw, *s_adv, *s_name, *s_pin, *s_sec, *s_txp;

static void set_str(char *dst, size_t cap, const char *src)
{ strncpy(dst, src ? src : "", cap - 1); dst[cap - 1] = '\0'; }

static void save_cb(lv_event_t *e)
{
    (void)e;
    AppConfig *cfg = appcfg_cache_peek();
    if (!cfg) return;

    cfg->bt.enabled   = lv_obj_has_state(s_sw, LV_STATE_CHECKED);
    cfg->bt.advertise = lv_obj_has_state(s_adv, LV_STATE_CHECKED);
    set_str(cfg->bt.legacy.name, sizeof(cfg->bt.legacy.name), lv_textarea_get_text(s_name));
    set_str(cfg->bt.legacy.pin,  sizeof(cfg->bt.legacy.pin),  lv_textarea_get_text(s_pin));
    cfg->bt.legacy.sec_mode = (app_bt_sec_mode_t)lv_dropdown_get_selected(s_sec);
    cfg->bt.tx_power        = (app_bt_tx_power_t)lv_dropdown_get_selected(s_txp);

    (void)appcfg_save(cfg);

#ifdef ESP_PLATFORM
    transport_ble_set_enabled(cfg->bt.enabled);
    if (cfg->bt.legacy.name[0]) transport_ble_set_name(cfg->bt.legacy.name);
    bool use_pk = (cfg->bt.legacy.sec_mode == APP_BT_SEC_PASSKEY) && cfg->bt.legacy.pin[0];
    transport_ble_set_passkey_mode(use_pk, use_pk ? (uint32_t)strtoul(cfg->bt.legacy.pin, NULL, 10) : 0);
    transport_ble_set_tx_power((uint8_t)cfg->bt.tx_power);
    if (cfg->bt.enabled && cfg->bt.advertise) transport_ble_start_adv();
    else                                      transport_ble_stop_adv();
#endif
    ui_nav_back();
}

void ui_netBleScreen_screen_init(void)
{
    const AppConfig *cfg = appcfg_cache_peek();
    lv_obj_t *content;
    ui_netBleScreen = ui_form_begin("Bluetooth LE", &content, save_cb);

    s_sw  = ui_form_switch(content, "Bluetooth habilitado", "app de servicio por BLE",
                           cfg && cfg->bt.enabled);
    s_adv = ui_form_switch(content, "Anunciando (visible)", "detectable para emparejar",
                           cfg && cfg->bt.advertise);
    s_name = ui_form_textarea(content, "NOMBRE DEL EQUIPO", cfg ? cfg->bt.legacy.name : "", false);
    s_pin  = ui_form_textarea(content, "PIN (6 dígitos)", cfg ? cfg->bt.legacy.pin : "", false);
    s_sec  = ui_form_dropdown(content, "SEGURIDAD", "Just Works\nPasskey (PIN)",
                              cfg ? (int)cfg->bt.legacy.sec_mode : 0);
    s_txp  = ui_form_dropdown(content, "POTENCIA TX",
                              "Muy bajo\nBajo\nBajo-medio\nMedio\nMedio-alto\nAlto\nMuy alto\nMáximo",
                              cfg ? (int)cfg->bt.tx_power : 3);

    /* acceso a la pantalla de emparejamiento por QR */
    lv_obj_t *btn = ui_card(content);
    lv_obj_set_width(btn, LV_PCT(100));
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_pad_ver(btn, 10, 0);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(btn, 8, 0);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn, ui_open_bleapp_cb, LV_EVENT_CLICKED, NULL);
    ui_icon(btn, UI_SYM_RADAR_2, UI_ICON_SM, UI_C_BLUE);
    ui_label(btn, "Configurar por app (QR de emparejamiento)", UI_FONT_SM, UI_C_BLUE);
}

void ui_netBleScreen_screen_destroy(void)
{
    if (ui_netBleScreen) { lv_obj_del(ui_netBleScreen); ui_netBleScreen = NULL; }
}

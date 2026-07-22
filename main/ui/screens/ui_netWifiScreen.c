#include "ui_netWifiScreen.h"
#include "ui_i18n.h"
#include "ui_form.h"
#include "ui_widgets.h"
#include "ui_nav.h"
#include "storage.h"
#include "ui_statusbar_controller.h"
#include <string.h>

#ifdef ESP_PLATFORM
#include "wifi_mgr.h"
#include "eth_mgr.h"
#endif

/* Configuración Wi-Fi (formulario). Sin mockup pixel-perfect: estilo del tema. */

lv_obj_t *ui_netWifiScreen = NULL;
static lv_obj_t *s_sw, *s_ssid, *s_pwd, *s_ipmode, *s_ip, *s_mask, *s_gw, *s_dns1, *s_dns2;

static void set_str(char *dst, size_t cap, const char *src)
{
    strncpy(dst, src ? src : "", cap - 1);
    dst[cap - 1] = '\0';
}

static void save_cb(lv_event_t *e)
{
    (void)e;
    AppConfig *cfg = appcfg_cache_peek();
    if (!cfg) return;

    cfg->wifi.enabled = lv_obj_has_state(s_sw, LV_STATE_CHECKED);
    set_str(cfg->wifi.ip_mode, sizeof(cfg->wifi.ip_mode),
            lv_dropdown_get_selected(s_ipmode) == 1 ? "static" : "dhcp");
    set_str(cfg->wifi.ssid, sizeof(cfg->wifi.ssid), lv_textarea_get_text(s_ssid));
    set_str(cfg->wifi.password, sizeof(cfg->wifi.password), lv_textarea_get_text(s_pwd));
    set_str(cfg->wifi.ip, sizeof(cfg->wifi.ip), lv_textarea_get_text(s_ip));
    set_str(cfg->wifi.mask, sizeof(cfg->wifi.mask), lv_textarea_get_text(s_mask));
    set_str(cfg->wifi.gw, sizeof(cfg->wifi.gw), lv_textarea_get_text(s_gw));
    set_str(cfg->wifi.dns1, sizeof(cfg->wifi.dns1), lv_textarea_get_text(s_dns1));
    set_str(cfg->wifi.dns2, sizeof(cfg->wifi.dns2), lv_textarea_get_text(s_dns2));

    (void)appcfg_save(cfg);

#ifdef ESP_PLATFORM
    (void)wifi_mgr_update_credentials(cfg->wifi.ssid, cfg->wifi.password);
    if (cfg->wifi.enabled) {
        if (eth_mgr_is_up()) wifi_mgr_stop();   /* Ethernet tiene prioridad */
        else                 wifi_mgr_start();
    } else {
        wifi_mgr_stop();
    }
#endif
    ui_statusbar_request_refresh();
    ui_nav_back();
}

void ui_netWifiScreen_screen_init(void)
{
    const AppConfig *cfg = appcfg_cache_peek();
    lv_obj_t *content;
    ui_netWifiScreen = ui_form_begin("Wi-Fi", &content, save_cb);

    s_sw = ui_form_switch(content, _t("Wi-Fi habilitado"), _t("conexión inalámbrica"),
                          cfg && cfg->wifi.enabled);
    s_ssid = ui_form_textarea(content, _t("SSID (RED)"), cfg ? cfg->wifi.ssid : "", false);
    s_pwd  = ui_form_textarea(content, _t("CONTRASEÑA"), cfg ? cfg->wifi.password : "", true);
    s_ipmode = ui_form_dropdown(content, _t("MODO IP"), _t("DHCP\nEstática"),
                                (cfg && strcmp(cfg->wifi.ip_mode, "static") == 0) ? 1 : 0);

    lv_obj_t *r1 = ui_form_row2(content);
    s_ip   = ui_form_textarea(r1, "IP", cfg ? cfg->wifi.ip : "", false);
    s_mask = ui_form_textarea(r1, _t("MÁSCARA"), cfg ? cfg->wifi.mask : "", false);
    lv_obj_set_flex_grow(lv_obj_get_parent(s_ip), 1);
    lv_obj_set_flex_grow(lv_obj_get_parent(s_mask), 1);

    lv_obj_t *r2 = ui_form_row2(content);
    s_gw   = ui_form_textarea(r2, "GATEWAY", cfg ? cfg->wifi.gw : "", false);
    s_dns1 = ui_form_textarea(r2, "DNS 1", cfg ? cfg->wifi.dns1 : "", false);
    lv_obj_set_flex_grow(lv_obj_get_parent(s_gw), 1);
    lv_obj_set_flex_grow(lv_obj_get_parent(s_dns1), 1);

    s_dns2 = ui_form_textarea(content, "DNS 2", cfg ? cfg->wifi.dns2 : "", false);
}

void ui_netWifiScreen_screen_destroy(void)
{
    if (ui_netWifiScreen) { lv_obj_del(ui_netWifiScreen); ui_netWifiScreen = NULL; }
}

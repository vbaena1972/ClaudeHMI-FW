#include "ui_netEthScreen.h"
#include "ui_i18n.h"
#include "ui_form.h"
#include "ui_widgets.h"
#include "ui_nav.h"
#include "storage.h"
#include "ui_statusbar_controller.h"
#include <string.h>

#ifdef ESP_PLATFORM
#include "eth_mgr.h"
#include "wifi_mgr.h"
#endif

lv_obj_t *ui_netEthScreen = NULL;
static lv_obj_t *s_sw, *s_hostname, *s_ipmode, *s_ip, *s_mask, *s_gw, *s_dns1, *s_dns2;

static void set_str(char *dst, size_t cap, const char *src)
{ strncpy(dst, src ? src : "", cap - 1); dst[cap - 1] = '\0'; }

static void save_cb(lv_event_t *e)
{
    (void)e;
    AppConfig *cfg = appcfg_cache_peek();
    if (!cfg) return;

    cfg->eth.enabled = lv_obj_has_state(s_sw, LV_STATE_CHECKED);
    set_str(cfg->eth.hostname, sizeof(cfg->eth.hostname), lv_textarea_get_text(s_hostname));
    set_str(cfg->eth.ip_mode, sizeof(cfg->eth.ip_mode),
            lv_dropdown_get_selected(s_ipmode) == 1 ? "static" : "dhcp");
    set_str(cfg->eth.ip, sizeof(cfg->eth.ip), lv_textarea_get_text(s_ip));
    set_str(cfg->eth.mask, sizeof(cfg->eth.mask), lv_textarea_get_text(s_mask));
    set_str(cfg->eth.gw, sizeof(cfg->eth.gw), lv_textarea_get_text(s_gw));
    set_str(cfg->eth.dns1, sizeof(cfg->eth.dns1), lv_textarea_get_text(s_dns1));
    set_str(cfg->eth.dns2, sizeof(cfg->eth.dns2), lv_textarea_get_text(s_dns2));

    (void)appcfg_save(cfg);

#ifdef ESP_PLATFORM
    if (cfg->eth.enabled) { eth_mgr_start(); wifi_mgr_stop(); }  /* Ethernet tiene prioridad */
    else                    eth_mgr_stop();
#endif
    ui_statusbar_request_refresh();
    ui_nav_back();
}

void ui_netEthScreen_screen_init(void)
{
    const AppConfig *cfg = appcfg_cache_peek();
    lv_obj_t *content;
    ui_netEthScreen = ui_form_begin("Ethernet", &content, save_cb);

    s_sw = ui_form_switch(content, _t("Ethernet habilitado"), _t("respaldo cableado (failover)"),
                          cfg && cfg->eth.enabled);
    s_hostname = ui_form_textarea(content, "HOSTNAME", cfg ? cfg->eth.hostname : "", false);
    s_ipmode = ui_form_dropdown(content, _t("MODO IP"), _t("DHCP\nEstática"),
                                (cfg && strcmp(cfg->eth.ip_mode, "static") == 0) ? 1 : 0);

    lv_obj_t *r1 = ui_form_row2(content);
    s_ip   = ui_form_textarea(r1, "IP", cfg ? cfg->eth.ip : "", false);
    s_mask = ui_form_textarea(r1, _t("MÁSCARA"), cfg ? cfg->eth.mask : "", false);
    lv_obj_set_flex_grow(lv_obj_get_parent(s_ip), 1);
    lv_obj_set_flex_grow(lv_obj_get_parent(s_mask), 1);

    lv_obj_t *r2 = ui_form_row2(content);
    s_gw   = ui_form_textarea(r2, "GATEWAY", cfg ? cfg->eth.gw : "", false);
    s_dns1 = ui_form_textarea(r2, "DNS 1", cfg ? cfg->eth.dns1 : "", false);
    lv_obj_set_flex_grow(lv_obj_get_parent(s_gw), 1);
    lv_obj_set_flex_grow(lv_obj_get_parent(s_dns1), 1);

    s_dns2 = ui_form_textarea(content, "DNS 2", cfg ? cfg->eth.dns2 : "", false);
}

void ui_netEthScreen_screen_destroy(void)
{
    if (ui_netEthScreen) { lv_obj_del(ui_netEthScreen); ui_netEthScreen = NULL; }
}

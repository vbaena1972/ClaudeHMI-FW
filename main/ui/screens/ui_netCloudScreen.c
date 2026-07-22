#include "ui_netCloudScreen.h"
#include "ui_i18n.h"
#include "ui_form.h"
#include "ui_widgets.h"
#include "ui_theme.h"
#include "ui_nav.h"
#include "storage.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef ESP_PLATFORM
#include "cert_store.h"
#endif

/* Configuración Nube / MQTT (formulario) + estado de certificados AWS. */

lv_obj_t *ui_netCloudScreen = NULL;
static lv_obj_t *s_sw, *s_type, *s_broker, *s_topic, *s_ota, *s_qos, *s_keep;
static lv_obj_t *s_cert_pills[3];   /* CA raíz, cert cliente, clave privada */

static void set_str(char *dst, size_t cap, const char *src)
{ strncpy(dst, src ? src : "", cap - 1); dst[cap - 1] = '\0'; }

/* ¿Está presente cada cert en NVS? (el sim muestra demo) */
static void cert_presence(bool present[3])
{
#ifdef ESP_PLATFORM
    cert_info_t info;
    present[0] = (cert_store_info(CERT_SERVER_CA,   &info) == ESP_OK) && info.present;
    present[1] = (cert_store_info(CERT_CLIENT_CERT, &info) == ESP_OK) && info.present;
    present[2] = (cert_store_info(CERT_CLIENT_KEY,  &info) == ESP_OK) && info.present;
#else
    present[0] = true; present[1] = true; present[2] = false;   /* demo del sim */
#endif
}

static void cert_pills_refresh(void)
{
    bool p[3]; cert_presence(p);
    for (int i = 0; i < 3; i++) {
        if (!s_cert_pills[i]) continue;
        lv_obj_t *txt = lv_obj_get_child(s_cert_pills[i], 0);
        lv_label_set_text(txt, p[i] ? "OK" : "falta");
        lv_obj_set_style_text_color(txt, ui_col(p[i] ? UI_C_OK_SOFT : UI_C_ALARM), 0);
        lv_obj_set_style_bg_color(s_cert_pills[i], ui_col(p[i] ? UI_C_OK_BG : UI_C_ALARM_BG), 0);
        lv_obj_set_style_border_color(s_cert_pills[i], ui_col(p[i] ? UI_C_OK_BORDER : UI_C_ALARM_BORDER), 0);
    }
}

static void cert_erase_cb(lv_event_t *e)
{
    (void)e;
#ifdef ESP_PLATFORM
    if (cert_store_erase_all() == ESP_OK)
        cert_pills_refresh();
#else
    cert_pills_refresh();
#endif
}

static void save_cb(lv_event_t *e)
{
    (void)e;
    AppConfig *cfg = appcfg_cache_peek();
    if (!cfg) return;

    cfg->cloud.enabled = lv_obj_has_state(s_sw, LV_STATE_CHECKED);
    set_str(cfg->cloud.type, sizeof(cfg->cloud.type),
            lv_dropdown_get_selected(s_type) == 1 ? "http" : "mqtt");
    set_str(cfg->cloud.broker_url, sizeof(cfg->cloud.broker_url), lv_textarea_get_text(s_broker));
    set_str(cfg->cloud.topic_base, sizeof(cfg->cloud.topic_base), lv_textarea_get_text(s_topic));
    set_str(cfg->cloud.ota_url, sizeof(cfg->cloud.ota_url), lv_textarea_get_text(s_ota));
    cfg->cloud.qos = atoi(lv_textarea_get_text(s_qos));
    cfg->cloud.keepalive = atoi(lv_textarea_get_text(s_keep));

    (void)appcfg_save(cfg);   /* la nube reconecta según config al arrancar net_core */
    ui_nav_back();
}

void ui_netCloudScreen_screen_init(void)
{
    const AppConfig *cfg = appcfg_cache_peek();
    lv_obj_t *content;
    ui_netCloudScreen = ui_form_begin("Nube · MQTT", &content, save_cb);

    s_sw = ui_form_switch(content, _t("Telemetría a la nube"), _t("publica presión/flujo por MQTT"),
                          cfg && cfg->cloud.enabled);
    s_type = ui_form_dropdown(content, _t("PROTOCOLO"), "MQTT\nHTTP",
                              (cfg && strcmp(cfg->cloud.type, "http") == 0) ? 1 : 0);
    s_broker = ui_form_textarea(content, "BROKER URL", cfg ? cfg->cloud.broker_url : "", false);
    s_topic  = ui_form_textarea(content, "TOPIC BASE", cfg ? cfg->cloud.topic_base : "", false);
    s_ota    = ui_form_textarea(content, "OTA URL", cfg ? cfg->cloud.ota_url : "", false);

    char qbuf[8], kbuf[8];
    snprintf(qbuf, sizeof(qbuf), "%d", cfg ? cfg->cloud.qos : 1);
    snprintf(kbuf, sizeof(kbuf), "%d", cfg ? cfg->cloud.keepalive : 60);
    lv_obj_t *r = ui_form_row2(content);
    s_qos  = ui_form_textarea(r, "QoS", qbuf, false);
    s_keep = ui_form_textarea(r, "KEEPALIVE (s)", kbuf, false);
    lv_obj_set_flex_grow(lv_obj_get_parent(s_qos), 1);
    lv_obj_set_flex_grow(lv_obj_get_parent(s_keep), 1);

    /* ===== Certificados AWS (importan solos al insertar la microSD: /sdcard/aws) ===== */
    lv_obj_t *cc = ui_card(content);
    lv_obj_set_width(cc, LV_PCT(100));
    lv_obj_set_style_radius(cc, 10, 0);
    lv_obj_set_style_pad_hor(cc, 12, 0);
    lv_obj_set_style_pad_ver(cc, 8, 0);
    lv_obj_set_flex_flow(cc, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(cc, 6, 0);
    lv_obj_t *cap = ui_label(cc, _t("CERTIFICADOS AWS (TLS)"), UI_FONT_XS, UI_C_TEXT_MUTED);
    lv_obj_set_style_text_letter_space(cap, 1, 0);

    static const char *CERT_LBL[3] = { "CA raíz", "Cert. cliente", "Clave privada" };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *row = ui_box(cc);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_t *lr = ui_box(row);
        lv_obj_set_size(lr, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(lr, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_column(lr, 6, 0);
        ui_icon(lr, UI_SYM_SHIELD_LOCK, UI_ICON_SM, UI_C_TEXT_3);
        ui_label(lr, _t(CERT_LBL[i]), UI_FONT_SM, UI_C_TEXT_2);
        s_cert_pills[i] = ui_pill(row, "--", UI_FONT_XS, UI_C_TEXT_2, UI_C_CARD_BG2, UI_C_BORDER);
    }
    cert_pills_refresh();

    lv_obj_t *note = ui_label(cc, _t("Se importan desde microSD: /sdcard/aws "
                              "(server_ca.pem · client_cert.pem · client_key.pem)"),
                              UI_FONT_XS, UI_C_TEXT_MUTED);
    lv_obj_set_width(note, LV_PCT(100));

    /* borrar certificados (acción destructiva: estilo alarm) */
    lv_obj_t *del = ui_box(cc);
    lv_obj_set_size(del, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(del, ui_col(UI_C_ALARM_BG), 0);
    lv_obj_set_style_bg_opa(del, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(del, 1, 0);
    lv_obj_set_style_border_color(del, ui_col(UI_C_ALARM_BORDER), 0);
    lv_obj_set_style_radius(del, UI_RADIUS_SM, 0);
    lv_obj_set_style_pad_ver(del, 8, 0);
    lv_obj_set_flex_flow(del, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(del, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(del, 6, 0);
    lv_obj_add_flag(del, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(del, cert_erase_cb, LV_EVENT_CLICKED, NULL);
    ui_label(del, _t("Borrar certificados"), UI_FONT_SM, UI_C_ALARM);
}

void ui_netCloudScreen_screen_destroy(void)
{
    if (ui_netCloudScreen) { lv_obj_del(ui_netCloudScreen); ui_netCloudScreen = NULL; }
}

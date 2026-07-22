#include "ui_netCloudScreen.h"
#include "ui_form.h"
#include "ui_widgets.h"
#include "ui_nav.h"
#include "storage.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Configuración Nube / MQTT (formulario). */

lv_obj_t *ui_netCloudScreen = NULL;
static lv_obj_t *s_sw, *s_type, *s_broker, *s_topic, *s_ota, *s_qos, *s_keep;

static void set_str(char *dst, size_t cap, const char *src)
{ strncpy(dst, src ? src : "", cap - 1); dst[cap - 1] = '\0'; }

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

    s_sw = ui_form_switch(content, "Telemetría a la nube", "publica presión/flujo por MQTT",
                          cfg && cfg->cloud.enabled);
    s_type = ui_form_dropdown(content, "PROTOCOLO", "MQTT\nHTTP",
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
}

void ui_netCloudScreen_screen_destroy(void)
{
    if (ui_netCloudScreen) { lv_obj_del(ui_netCloudScreen); ui_netCloudScreen = NULL; }
}

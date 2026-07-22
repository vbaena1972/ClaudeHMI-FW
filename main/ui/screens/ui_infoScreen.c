#include "ui_infoScreen.h"
#include "ui_i18n.h"
#include "ui_widgets.h"
#include "ui_theme.h"
#include "ui_nav.h"
#include "storage.h"
#include "metrics_store.h"
#include <string.h>
#include <stdio.h>

/* Información del equipo (mockup 4f). */

lv_obj_t *ui_infoScreen = NULL;

static int32_t s_kv_col[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
static int32_t s_kv_row[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
static int32_t s_bot_col[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
static int32_t s_bot_row[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

/* tarjeta clave-valor */
static lv_obj_t *kv_card(lv_obj_t *parent, const char *label, const char *value, uint32_t vcol)
{
    lv_obj_t *c = ui_card(parent);
    lv_obj_set_style_radius(c, 10, 0);
    lv_obj_set_style_pad_hor(c, 11, 0);
    lv_obj_set_style_pad_ver(c, 7, 0);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(c, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(c, 2, 0);
    lv_obj_t *l = ui_label(c, label, UI_FONT_XS, UI_C_TEXT_MUTED);
    lv_obj_set_style_text_letter_space(l, 1, 0);
    ui_label(c, value, UI_FONT_MD, vcol);
    return c;
}

static const char *gas_label(const char *g)
{
    if (g && strcmp(g, "air_med") == 0) return _t("Aire med. (AIR)");
    if (g && strcmp(g, "n2o") == 0)     return _t("Óxido nitroso (N₂O)");
    if (g && strcmp(g, "vac") == 0)     return _t("Vacío (VAC)");
    return _t("Oxígeno (O₂)");
}

void ui_infoScreen_screen_init(void)
{
    const AppConfig *cfg = appcfg_cache_peek();
    ui_infoScreen = ui_screen_base();
    lv_obj_set_flex_flow(ui_infoScreen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(ui_infoScreen, 8, 0);
    lv_obj_set_style_pad_row(ui_infoScreen, 8, 0);

    /* header: back + título + badge normativa */
    lv_obj_t *hdr = ui_box(ui_infoScreen);
    lv_obj_set_size(hdr, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *hb = ui_box(hdr);
    lv_obj_set_size(hb, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hb, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hb, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(hb, 9, 0);
    lv_obj_t *back = ui_icon_badge(hb, UI_SYM_ARROW_LEFT, UI_ICON_SM, UI_C_TEXT, UI_C_CARD_BG, 28);
    lv_obj_add_flag(back, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(back, ui_nav_back_event_cb, LV_EVENT_CLICKED, NULL);
    ui_label(hb, _t("Información del equipo"), UI_FONT_TITLE, UI_C_TEXT);

    lv_obj_t *badge = ui_pill(hdr, "", UI_FONT_XS, UI_C_OK_SOFT, UI_C_OK_BG, UI_C_OK_BORDER);
    lv_obj_set_flex_flow(badge, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(badge, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(badge, 5, 0);
    lv_obj_t *bl = lv_obj_get_child(badge, 0);
    lv_label_set_text(bl, "NFPA 99 · ISO 7396-1");
    lv_obj_t *bi = ui_icon(badge, UI_SYM_SHIELD_CHECK, UI_ICON_SM, UI_C_OK_SOFT);
    lv_obj_move_to_index(bi, 0);

    /* fila: tarjeta de modelo + grid 2x2 */
    lv_obj_t *toprow = ui_box(ui_infoScreen);
    lv_obj_set_size(toprow, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(toprow, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(toprow, 10, 0);

    lv_obj_t *dev = ui_card(toprow);
    lv_obj_set_style_pad_hor(dev, 18, 0);
    lv_obj_set_style_pad_ver(dev, 12, 0);
    lv_obj_set_flex_flow(dev, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(dev, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *db = ui_icon_badge(dev, UI_SYM_ACTIVITY, UI_ICON_LG, 0x9fe1cb, UI_C_HEADER_ICONBG, 44);
    lv_obj_set_style_radius(db, 12, 0);
    lv_obj_t *dm = ui_label(dev, cfg && cfg->general.model[0] ? cfg->general.model : "MPF-4200",
                            UI_FONT_MD, UI_C_TEXT);
    lv_obj_set_style_margin_top(dm, 8, 0);
    ui_label(dev, cfg && cfg->general.info_text[0] ? cfg->general.info_text : "Axira Equipos",
             UI_FONT_XS, UI_C_TEXT_MUTED);

    lv_obj_t *kvg = ui_box(toprow);
    lv_obj_set_flex_grow(kvg, 1);
    lv_obj_set_height(kvg, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_column(kvg, 7, 0);
    lv_obj_set_style_pad_row(kvg, 7, 0);
    lv_obj_set_grid_dsc_array(kvg, s_kv_col, s_kv_row);
    lv_obj_set_layout(kvg, LV_LAYOUT_GRID);
    lv_obj_t *k;
    k = kv_card(kvg, _t("N.º DE SERIE"), cfg && cfg->general.serial[0] ? cfg->general.serial : "AX-4200-01847", UI_C_TEXT);
    lv_obj_set_grid_cell(k, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    k = kv_card(kvg, _t("GAS CONFIGURADO"), gas_label(cfg ? cfg->sensors.gas_type : "o2"), UI_C_OK);
    lv_obj_set_grid_cell(k, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    char verbuf[24];
    snprintf(verbuf, sizeof(verbuf), "v%s",
             cfg && cfg->general.hw_version[0] ? cfg->general.hw_version : "-");
    k = kv_card(kvg, _t("VERSIÓN HW"), verbuf, UI_C_TEXT);
    lv_obj_set_grid_cell(k, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    snprintf(verbuf, sizeof(verbuf), "v%s",
             cfg && cfg->general.fw_version[0] ? cfg->general.fw_version : "-");
    k = kv_card(kvg, _t("FW APLICACIÓN"), verbuf, UI_C_TEXT);
    lv_obj_set_grid_cell(k, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

    /* grid inferior 3x1 */
    lv_obj_t *botg = ui_box(ui_infoScreen);
    lv_obj_set_width(botg, LV_PCT(100));
    lv_obj_set_flex_grow(botg, 1);
    lv_obj_set_style_pad_column(botg, 7, 0);
    lv_obj_set_grid_dsc_array(botg, s_bot_col, s_bot_row);
    lv_obj_set_layout(botg, LV_LAYOUT_GRID);
    /* Tiempo en servicio: minutos acumulados en NVS (metrics_store) -> días */
    char svc[24];
    uint32_t svc_min = appmetrics_service_min();
    if (svc_min >= 60)
        snprintf(svc, sizeof(svc), "%lu %s", (unsigned long)(svc_min / 1440u), _t("días"));
    else
        snprintf(svc, sizeof(svc), "\xE2\x80\x94");   /* aún sin datos */
    k = kv_card(botg, _t("TIEMPO EN SERVICIO"), svc, UI_C_TEXT);
    lv_obj_set_grid_cell(k, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    k = kv_card(botg, _t("ÚLTIMA CALIBRACIÓN"),
                cfg && cfg->sensors.cal.last_cal_date[0] ? cfg->sensors.cal.last_cal_date : "\xE2\x80\x94",
                UI_C_TEXT);
    lv_obj_set_grid_cell(k, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    k = kv_card(botg, _t("PRÓX. MANTENIM."),
                cfg && cfg->sensors.cal.next_service_date[0] ? cfg->sensors.cal.next_service_date : "\xE2\x80\x94",
                UI_C_WARN_SOFT);
    lv_obj_set_grid_cell(k, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    /* pie con QR */
    lv_obj_t *foot = ui_box(ui_infoScreen);
    lv_obj_set_size(foot, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(foot, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(foot, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(foot, 8, 0);
    ui_icon(foot, UI_SYM_QRCODE, UI_ICON_SM, 0x4a5058);
    ui_label(foot, _t("Escanea para historial y manual · soporte@axira.com"), UI_FONT_XS, 0x4a5058);
}

void ui_infoScreen_screen_destroy(void)
{
    if (ui_infoScreen) { lv_obj_del(ui_infoScreen); ui_infoScreen = NULL; }
}

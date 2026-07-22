#include "ui_sensorDiagScreen.h"
#include "ui_widgets.h"
#include "ui_theme.h"

/* Diagnóstico de sensores (mockup 4d). */

lv_obj_t *ui_sensorDiagScreen = NULL;

static lv_obj_t *kvcol(lv_obj_t *parent, const char *cap, const char *val)
{
    lv_obj_t *c = ui_box(parent);
    lv_obj_set_size(c, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(c, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_t *l = ui_label(c, cap, UI_FONT_XS, UI_C_TEXT_MUTED);
    lv_obj_set_style_text_letter_space(l, 1, 0);
    ui_label(c, val, UI_FONT_SM, 0xcfd3d9);
    return c;
}

static void sensor_row(lv_obj_t *parent, const char *sym, uint32_t icon_col,
                       const char *name, const char *desc,
                       const char *raw, const char *cal,
                       const char *status, uint32_t status_col)
{
    lv_obj_t *r = ui_card(parent);
    lv_obj_set_width(r, LV_PCT(100));
    lv_obj_set_flex_grow(r, 1);
    lv_obj_set_style_radius(r, UI_RADIUS_TILE, 0);
    lv_obj_set_style_pad_hor(r, 14, 0);
    lv_obj_set_style_pad_ver(r, 0, 0);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(r, 12, 0);

    lv_obj_t *badge = lv_obj_create(r);
    ui_kill_scroll(badge);
    lv_obj_set_size(badge, 34, 34);
    lv_obj_set_style_radius(badge, 9, 0);
    lv_obj_set_style_bg_color(badge, ui_col(icon_col), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_20, 0);
    lv_obj_center(ui_icon(badge, sym, UI_ICON_MD, icon_col));

    lv_obj_t *nc = ui_box(r);
    lv_obj_set_flex_grow(nc, 1);
    lv_obj_set_height(nc, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(nc, LV_FLEX_FLOW_COLUMN);
    ui_label(nc, name, UI_FONT_MD, UI_C_TEXT);
    ui_label(nc, desc, UI_FONT_XS, UI_C_TEXT_MUTED);

    kvcol(r, "CRUDO", raw);
    kvcol(r, "CAL.", cal);

    lv_obj_t *pill = ui_box(r);
    lv_obj_set_size(pill, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(pill, ui_col(status_col), 0);
    lv_obj_set_style_bg_opa(pill, LV_OPA_10, 0);
    lv_obj_set_style_radius(pill, UI_RADIUS_SM, 0);
    lv_obj_set_style_border_width(pill, 1, 0);
    lv_obj_set_style_border_color(pill, ui_col(status_col), 0);
    lv_obj_set_style_border_opa(pill, LV_OPA_30, 0);
    lv_obj_set_style_pad_hor(pill, 10, 0);
    lv_obj_set_style_pad_ver(pill, 4, 0);
    lv_obj_set_flex_flow(pill, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pill, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(pill, 5, 0);
    lv_obj_t *dot = ui_box(pill);
    lv_obj_set_size(dot, 6, 6);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, ui_col(status_col), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    ui_label(pill, status, UI_FONT_XS, status_col);
}

static lv_obj_t *action_btn(lv_obj_t *parent, const char *sym, const char *txt,
                            uint32_t bg, uint32_t border, uint32_t txt_col)
{
    lv_obj_t *b = ui_box(parent);
    lv_obj_set_flex_grow(b, 1);
    lv_obj_set_height(b, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(b, ui_col(bg), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(b, UI_RADIUS_SM, 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_style_border_color(b, ui_col(border), 0);
    lv_obj_set_style_pad_ver(b, 7, 0);
    lv_obj_set_flex_flow(b, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(b, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(b, 7, 0);
    lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
    ui_icon(b, sym, UI_ICON_SM, txt_col);
    ui_label(b, txt, UI_FONT_SM, txt_col);
    return b;
}

void ui_sensorDiagScreen_screen_init(void)
{
    ui_sensorDiagScreen = ui_screen_base();
    lv_obj_set_flex_flow(ui_sensorDiagScreen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(ui_sensorDiagScreen, 8, 0);
    lv_obj_set_style_pad_row(ui_sensorDiagScreen, 7, 0);

    lv_obj_t *hdr = ui_nav_header(ui_sensorDiagScreen, "Sensores");
    ui_pill(hdr, "3 activos · 0 fallas", UI_FONT_XS, UI_C_OK_DIM, UI_C_OK_BG, UI_C_OK_BORDER);

    lv_obj_t *rows = ui_box(ui_sensorDiagScreen);
    lv_obj_set_width(rows, LV_PCT(100));
    lv_obj_set_flex_grow(rows, 1);
    lv_obj_set_flex_flow(rows, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(rows, 7, 0);

    sensor_row(rows, UI_SYM_GAUGE, UI_C_OK, "Presión · P-01", "transductor 0–2200 psi · I²C 0x28",
               "3.412 V · 1400 psi", "12 mar 2026", "OK", UI_C_OK);
    sensor_row(rows, UI_SYM_WIND, UI_C_OK, "Flujo · F-01", "másico térmico 0–1500 SCCM · UART",
               "2.870 V · 889 SCCM", "12 mar 2026", "OK", UI_C_OK);
    sensor_row(rows, UI_SYM_TEMPERATURE, UI_C_WARN_SOFT, "Temp. de línea · T-01", "compensación de flujo · on-die",
               "— · 23.4 °C", "fábrica", "DERIVA", UI_C_WARN_SOFT);

    lv_obj_t *acts = ui_box(ui_sensorDiagScreen);
    lv_obj_set_size(acts, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(acts, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(acts, 9, 0);
    action_btn(acts, UI_SYM_REFRESH, "Recalibrar cero", UI_C_CARD_BG, 0x2c313a, 0xcfd3d9);
    action_btn(acts, UI_SYM_FILE_EXPORT, "Exportar log de sensores", UI_C_OK_BG, UI_C_OK_BORDER, UI_C_OK_SOFT);
}

void ui_sensorDiagScreen_screen_destroy(void)
{
    if (ui_sensorDiagScreen) { lv_obj_del(ui_sensorDiagScreen); ui_sensorDiagScreen = NULL; }
}

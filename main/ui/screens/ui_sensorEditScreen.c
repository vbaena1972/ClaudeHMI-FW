#include "ui_sensorEditScreen.h"
#include "ui_widgets.h"
#include "ui_theme.h"
#include "ui.h"
#include "ui_cfg.h"
#include <string.h>
#include <stdio.h>

/* Sensores y alarmas — edición (mockup 5b), con persistencia en NVS. */

lv_obj_t *ui_sensorEditScreen = NULL;
static lv_obj_t *s_gas_lbl = NULL;

static int32_t s_col[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
static int32_t s_row[] = { LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

/* --- unidades: etiquetas visibles y claves guardadas --- */
static const char *PU_LBL[] = { "PSI", "BAR", "kPa", "MPa" };
static const char *PU_KEY[] = { "psi", "bar", "kpa", "mpa" };
static const char *FU_LBL[] = { "LPM", "SCCM", "m³/h" };
static const char *FU_KEY[] = { "lpm", "sccm", "m3h" };

static int idx_of(const char *val, const char *const *keys, int n)
{
    for (int i = 0; i < n; i++) if (strcmp(val, keys[i]) == 0) return i;
    return 0;
}

/* --- gas: claves y etiquetas para ciclar --- */
static const char *GAS_KEY[] = { "o2", "air_med", "n2o", "vac" };
static const char *GAS_LBL[] = { "Oxígeno · O₂", "Aire med. · AIR", "Óxido nitroso · N₂O", "Vacío · VAC" };

static lv_obj_t *field_card(lv_obj_t *parent, const char *caption)
{
    lv_obj_t *c = ui_card(parent);
    lv_obj_set_style_radius(c, 10, 0);
    lv_obj_set_style_pad_hor(c, 12, 0);
    lv_obj_set_style_pad_ver(c, 7, 0);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(c, 5, 0);
    lv_obj_t *l = ui_label(c, caption, UI_FONT_XS, UI_C_TEXT_MUTED);
    lv_obj_set_style_text_letter_space(l, 1, 0);
    return c;
}

/* ---- control segmentado (guarda la unidad al tocar) ---- */
static void seg_apply_highlight(lv_obj_t *row, lv_obj_t *active)
{
    uint32_t n = lv_obj_get_child_count(row);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *o = lv_obj_get_child(row, i);
        bool act = (o == active);
        lv_obj_set_style_bg_opa(o, act ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_set_style_text_color(lv_obj_get_child(o, 0), ui_col(act ? 0x06251f : UI_C_TEXT_2), 0);
    }
}

static void seg_click_cb(lv_event_t *e)
{
    lv_obj_t *opt = lv_event_get_target(e);
    lv_obj_t *row = lv_obj_get_parent(opt);
    seg_apply_highlight(row, opt);
    const char *key = (const char *)lv_obj_get_user_data(opt);
    int kind = (int)(intptr_t)lv_obj_get_user_data(row);
    if (key) {
        if (kind == 0) ui_cfg_set_pressure_unit(key);
        else           ui_cfg_set_flow_unit(key);
    }
}

static void seg_control(lv_obj_t *parent, const char *const *lbls, const char *const *keys,
                        int n, int active, int kind)
{
    lv_obj_t *row = ui_box(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, 4, 0);
    lv_obj_set_user_data(row, (void *)(intptr_t)kind);
    for (int i = 0; i < n; i++) {
        lv_obj_t *o = ui_box(row);
        lv_obj_set_flex_grow(o, 1);
        lv_obj_set_height(o, LV_SIZE_CONTENT);
        lv_obj_set_style_radius(o, UI_RADIUS_PILL, 0);
        lv_obj_set_style_pad_ver(o, 5, 0);
        bool act = (i == active);
        lv_obj_set_style_bg_color(o, ui_col(UI_C_OK), 0);
        lv_obj_set_style_bg_opa(o, act ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_set_user_data(o, (void *)keys[i]);
        lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(o, seg_click_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *l = ui_label(o, lbls[i], UI_FONT_SM, act ? 0x06251f : UI_C_TEXT_2);
        lv_obj_center(l);
    }
}

/* ---- gas: ciclar al tocar la tarjeta ---- */
static void gas_click_cb(lv_event_t *e)
{
    (void)e;
    int cur = idx_of(ui_cfg_gas(), GAS_KEY, 4);
    int nxt = (cur + 1) % 4;
    ui_cfg_set_gas(GAS_KEY[nxt]);
    if (s_gas_lbl) lv_label_set_text(s_gas_lbl, GAS_LBL[nxt]);
}

/* ---- umbral: fija el objetivo y abre el keypad ---- */
static void thr_cb(lv_event_t *e)
{
    ui_edit_target_t tgt = (ui_edit_target_t)(intptr_t)lv_event_get_user_data(e);
    ui_edit_begin(tgt);
    ui_open_keypad_cb(e);
}

static lv_obj_t *thr_box(lv_obj_t *parent, const char *label, float value, const char *unit,
                         ui_edit_target_t target)
{
    lv_obj_t *b = ui_box(parent);
    lv_obj_set_flex_grow(b, 1);
    lv_obj_set_height(b, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(b, ui_col(0x12181f), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(b, UI_RADIUS_SM, 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_style_border_color(b, ui_col(UI_C_ALARM), 0);
    lv_obj_set_style_border_opa(b, LV_OPA_40, 0);
    lv_obj_set_style_pad_hor(b, 12, 0);
    lv_obj_set_style_pad_ver(b, 6, 0);
    lv_obj_set_flex_flow(b, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(b, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(b, thr_cb, LV_EVENT_CLICKED, (void *)(intptr_t)target);
    ui_label(b, label, UI_FONT_SM, 0xf6a5a5);
    lv_obj_t *vw = ui_box(b);
    lv_obj_set_size(vw, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(vw, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(vw, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(vw, 4, 0);
    char buf[16]; snprintf(buf, sizeof(buf), "%.0f", value);
    ui_label(vw, buf, UI_FONT_LG, UI_C_ALARM);
    ui_label(vw, unit, UI_FONT_XS, UI_C_TEXT_2);
    return b;
}

void ui_sensorEditScreen_screen_init(void)
{
    const char *pu = ui_cfg_pressure_unit();
    const char *fu = ui_cfg_flow_unit();

    ui_sensorEditScreen = ui_screen_base();
    lv_obj_set_flex_flow(ui_sensorEditScreen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(ui_sensorEditScreen, 8, 0);
    lv_obj_set_style_pad_row(ui_sensorEditScreen, 7, 0);

    lv_obj_t *hdr = ui_nav_header(ui_sensorEditScreen, "Sensores y alarmas");
    lv_obj_t *pill = ui_pill(hdr, "", UI_FONT_XS, UI_C_OK_SOFT, UI_C_OK_BG, UI_C_OK_BORDER);
    lv_obj_set_flex_flow(pill, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(pill, 5, 0);
    lv_label_set_text(lv_obj_get_child(pill, 0), "desbloqueado");
    lv_obj_t *pi = ui_icon(pill, UI_SYM_LOCK_OPEN, UI_ICON_SM, UI_C_OK_SOFT);
    lv_obj_move_to_index(pi, 0);

    lv_obj_t *grid = ui_box(ui_sensorEditScreen);
    lv_obj_set_width(grid, LV_PCT(100));
    lv_obj_set_flex_grow(grid, 1);
    lv_obj_set_style_pad_column(grid, 8, 0);
    lv_obj_set_style_pad_row(grid, 8, 0);
    lv_obj_set_grid_dsc_array(grid, s_col, s_row);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);

    /* TIPO DE GAS (clic = ciclar) */
    lv_obj_t *gas = field_card(grid, "TIPO DE GAS");
    lv_obj_set_grid_cell(gas, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_flex_flow(gas, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(gas, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(gas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(gas, gas_click_cb, LV_EVENT_CLICKED, NULL);
    s_gas_lbl = ui_label(gas, GAS_LBL[idx_of(ui_cfg_gas(), GAS_KEY, 4)], UI_FONT_LG, UI_C_OK);
    ui_icon(gas, UI_SYM_CHEVRON_DOWN, UI_ICON_SM, UI_C_TEXT_3);

    /* CÓDIGO DE COLOR */
    lv_obj_t *cc = field_card(grid, "CÓDIGO DE COLOR");
    lv_obj_set_grid_cell(cc, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_flex_flow(cc, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cc, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    ui_label(cc, "NFPA · verde/blanco", UI_FONT_LG, UI_C_TEXT);
    lv_obj_t *sw = ui_box(cc);
    lv_obj_set_size(sw, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(sw, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(sw, 3, 0);
    for (int i = 0; i < 2; i++) {
        lv_obj_t *s = ui_box(sw);
        lv_obj_set_size(s, 11, 16);
        lv_obj_set_style_radius(s, 2, 0);
        lv_obj_set_style_bg_color(s, ui_col(i ? 0xe6e8eb : UI_C_OK), 0);
        lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
    }

    /* UNIDAD DE PRESIÓN */
    lv_obj_t *up = field_card(grid, "UNIDAD DE PRESIÓN");
    lv_obj_set_grid_cell(up, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    seg_control(up, PU_LBL, PU_KEY, 4, idx_of(pu, PU_KEY, 4), 0);

    /* UNIDAD DE FLUJO */
    lv_obj_t *uf = field_card(grid, "UNIDAD DE FLUJO");
    lv_obj_set_grid_cell(uf, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    seg_control(uf, FU_LBL, FU_KEY, 3, idx_of(fu, FU_KEY, 3), 1);

    /* UMBRALES (span 2 columnas) */
    lv_obj_t *thr = field_card(grid, "UMBRALES DE ALARMA · PRESIÓN (toca para editar)");
    lv_obj_set_grid_cell(thr, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 2, 1);
    lv_obj_t *thr_row = ui_box(thr);
    lv_obj_set_size(thr_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(thr_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(thr_row, 8, 0);
    thr_box(thr_row, "Mínimo", ui_cfg_pressure_min(), pu, UI_EDIT_PRES_MIN);
    thr_box(thr_row, "Máximo", ui_cfg_pressure_max(), pu, UI_EDIT_PRES_MAX);

    /* footer */
    lv_obj_t *foot = ui_box(ui_sensorEditScreen);
    lv_obj_set_size(foot, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(foot, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(foot, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(foot, 6, 0);
    ui_icon(foot, UI_SYM_DEVICE_MOBILE_MESSAGE, UI_ICON_SM, UI_C_TEXT_MUTED);
    ui_label(foot, "SSID, contraseñas y certificados AWS se configuran desde la app por BLE",
             UI_FONT_XS, UI_C_TEXT_MUTED);
}

void ui_sensorEditScreen_screen_destroy(void)
{
    if (ui_sensorEditScreen) { lv_obj_del(ui_sensorEditScreen); ui_sensorEditScreen = NULL; s_gas_lbl = NULL; }
}

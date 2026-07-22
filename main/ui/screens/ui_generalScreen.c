#include "ui_generalScreen.h"
#include "ui_i18n.h"
#include "ui_widgets.h"
#include "ui_theme.h"
#include "ui.h"
#include "ui_nav.h"
#include "ui_cfg.h"
#include <string.h>

/* Menú de ajustes generales (mockup 4e): grid 2x3 de tiles. */

lv_obj_t *ui_generalScreen = NULL;

/* Labels de valor que se refrescan al mostrar la pantalla (se crea una sola vez). */
static lv_obj_t *s_val_brightness = NULL;
static lv_obj_t *s_val_units = NULL;

static void general_loaded_cb(lv_event_t *e)
{
    (void)e;
    if (s_val_brightness)
        lv_label_set_text_fmt(s_val_brightness, "%d%%", ui_cfg_brightness());
    if (s_val_units) {
        char p[8], f[8];
        strncpy(p, ui_cfg_pressure_unit(), sizeof(p) - 1); p[sizeof(p) - 1] = '\0';
        strncpy(f, ui_cfg_flow_unit(),     sizeof(f) - 1); f[sizeof(f) - 1] = '\0';
        lv_label_set_text_fmt(s_val_units, "%s \xC2\xB7 %s", p, f);
    }
}

static int32_t s_col_dsc[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
static int32_t s_row_dsc[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

static lv_obj_t *gen_tile(lv_obj_t *grid, const char *sym, const char *title, const char *sub,
                          const char *value, uint32_t icon_col, uint32_t value_col,
                          int col, int row, lv_event_cb_t cb)
{
    lv_obj_t *tile = ui_card(grid);
    lv_obj_set_grid_cell(tile, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_set_style_radius(tile, UI_RADIUS_TILE, 0);
    lv_obj_set_style_pad_hor(tile, 12, 0);
    lv_obj_set_style_pad_ver(tile, 0, 0);
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(tile, 10, 0);
    lv_obj_add_flag(tile, LV_OBJ_FLAG_CLICKABLE);
    if (cb) lv_obj_add_event_cb(tile, cb, LV_EVENT_CLICKED, NULL);

    /* badge de icono tintado (color con opacidad baja) */
    lv_obj_t *badge = lv_obj_create(tile);
    ui_kill_scroll(badge);
    lv_obj_set_size(badge, 32, 32);
    lv_obj_set_style_radius(badge, UI_RADIUS_SM, 0);
    lv_obj_set_style_bg_color(badge, ui_col(icon_col), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_20, 0);
    lv_obj_t *ic = ui_icon(badge, sym, UI_ICON_MD, icon_col);
    lv_obj_center(ic);

    lv_obj_t *col_c = ui_box(tile);
    lv_obj_set_flex_grow(col_c, 1);
    lv_obj_set_height(col_c, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(col_c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(col_c, 1, 0);
    ui_label(col_c, title, UI_FONT_MD, UI_C_TEXT);
    if (sub) ui_label(col_c, sub, UI_FONT_XS, UI_C_TEXT_MUTED);

    lv_obj_t *val = value ? ui_label(tile, value, UI_FONT_SM, value_col) : NULL;
    ui_icon(tile, UI_SYM_CHEVRON_RIGHT, UI_ICON_SM, UI_C_TEXT_MUTED);
    return val; /* label de valor (o NULL) por si el caller quiere refrescarlo */
}

void ui_generalScreen_screen_init(void)
{
    ui_generalScreen = ui_screen_base();
    lv_obj_set_flex_flow(ui_generalScreen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(ui_generalScreen, 8, 0);
    lv_obj_set_style_pad_row(ui_generalScreen, 8, 0);

    /* header con back + subtítulo a la derecha */
    lv_obj_t *hdr = ui_box(ui_generalScreen);
    lv_obj_set_size(hdr, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *back;
    lv_obj_t *hb = ui_box(hdr);
    lv_obj_set_size(hb, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hb, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hb, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(hb, 9, 0);
    back = ui_icon_badge(hb, UI_SYM_ARROW_LEFT, UI_ICON_SM, UI_C_TEXT, UI_C_CARD_BG, 28);
    lv_obj_add_flag(back, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(back, ui_nav_back_event_cb, LV_EVENT_CLICKED, NULL);
    ui_label(hb, _t("Ajustes generales"), UI_FONT_TITLE, UI_C_TEXT);
    ui_label(hdr, "UCI Torre A", UI_FONT_XS, UI_C_TEXT_3);

    /* grid 2x3 */
    lv_obj_t *grid = ui_box(ui_generalScreen);
    lv_obj_set_width(grid, LV_PCT(100));
    lv_obj_set_flex_grow(grid, 1);
    lv_obj_set_style_pad_column(grid, 8, 0);
    lv_obj_set_style_pad_row(grid, 8, 0);
    lv_obj_set_grid_dsc_array(grid, s_col_dsc, s_row_dsc);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);

    s_val_brightness = gen_tile(grid, UI_SYM_BRIGHTNESS_UP, _t("Pantalla"), _t("brillo · reposo"), "80%",
             UI_C_BLUE, UI_C_TEXT_2, 0, 0, ui_open_general_simple_cb);
    gen_tile(grid, UI_SYM_BELL, _t("Alarmas"), _t("umbrales · buzzer"), "NFPA 99",
             UI_C_ALARM, UI_C_TEXT_2, 1, 0, ui_open_sensordiag_cb);
    s_val_units = gen_tile(grid, UI_SYM_RULER_2, _t("Unidades"), _t("presión · flujo"), "psi · SCCM",
             UI_C_OK, UI_C_TEXT_2, 0, 1, ui_open_sensor_cb);
    gen_tile(grid, UI_SYM_WIFI, _t("Red y nube"), "wifi · MQTT", _t("conectado"),
             UI_C_TEAL, UI_C_OK, 1, 1, ui_open_connectivity_cb);
    gen_tile(grid, UI_SYM_CLOCK, _t("Fecha y hora"), _t("zona · NTP"), "GMT-5",
             UI_C_WARN_SOFT, UI_C_TEXT_2, 0, 2, ui_open_general_simple_cb);
    gen_tile(grid, UI_SYM_INFO_CIRCLE, _t("Información"), _t("firmware · serie"), NULL,
             UI_C_TEXT_2, UI_C_TEXT_2, 1, 2, ui_open_info_cb);

    lv_obj_add_event_cb(ui_generalScreen, general_loaded_cb, LV_EVENT_SCREEN_LOADED, NULL);
    general_loaded_cb(NULL); /* valores reales desde la primera carga */
}

void ui_generalScreen_screen_destroy(void)
{
    if (ui_generalScreen) { lv_obj_del(ui_generalScreen); ui_generalScreen = NULL;
                            s_val_brightness = NULL; s_val_units = NULL; }
}

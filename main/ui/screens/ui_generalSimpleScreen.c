#include "ui_generalSimpleScreen.h"
#include "ui_widgets.h"
#include "ui_theme.h"

/* Ajustes generales (modo simple / local, mockup 5e). */

lv_obj_t *ui_generalSimpleScreen = NULL;

static lv_obj_t *simple_row(lv_obj_t *parent, const char *sym, uint32_t icon_col,
                            const char *title, const char *sub)
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
    lv_obj_set_size(badge, 32, 32);
    lv_obj_set_style_radius(badge, UI_RADIUS_SM, 0);
    lv_obj_set_style_bg_color(badge, ui_col(icon_col), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_20, 0);
    lv_obj_center(ui_icon(badge, sym, UI_ICON_MD, icon_col));

    lv_obj_t *col = ui_box(r);
    lv_obj_set_flex_grow(col, 1);
    lv_obj_set_height(col, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    ui_label(col, title, UI_FONT_MD, UI_C_TEXT);
    ui_label(col, sub, UI_FONT_XS, UI_C_TEXT_MUTED);
    return r;
}

void ui_generalSimpleScreen_screen_init(void)
{
    ui_generalSimpleScreen = ui_screen_base();
    lv_obj_set_flex_flow(ui_generalSimpleScreen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(ui_generalSimpleScreen, 8, 0);
    lv_obj_set_style_pad_row(ui_generalSimpleScreen, 8, 0);

    lv_obj_t *hdr = ui_nav_header(ui_generalSimpleScreen, "Ajustes generales");
    ui_label(hdr, "local · sin red", UI_FONT_XS, UI_C_TEXT_3);

    lv_obj_t *rows = ui_box(ui_generalSimpleScreen);
    lv_obj_set_width(rows, LV_PCT(100));
    lv_obj_set_flex_grow(rows, 1);
    lv_obj_set_flex_flow(rows, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(rows, 8, 0);

    /* Idioma */
    lv_obj_t *lang = simple_row(rows, UI_SYM_LANGUAGE, UI_C_OK, "Idioma", "idioma de la interfaz");
    ui_label(lang, "Español", UI_FONT_SM, 0xe6e8eb);
    ui_icon(lang, UI_SYM_CHEVRON_RIGHT, UI_ICON_SM, UI_C_TEXT_MUTED);

    /* Zona horaria */
    lv_obj_t *tz = simple_row(rows, UI_SYM_CLOCK, UI_C_WARN_SOFT, "Zona horaria",
                              "RTC RV-3028 · hora local de eventos");
    ui_label(tz, "GMT-5 Bogotá", UI_FONT_SM, 0xe6e8eb);
    ui_icon(tz, UI_SYM_CHEVRON_RIGHT, UI_ICON_SM, UI_C_TEXT_MUTED);

    /* Brillo con slider */
    lv_obj_t *br = simple_row(rows, UI_SYM_BRIGHTNESS_UP, UI_C_BLUE, "Brillo",
                             "reposo al 50% tras inactividad");
    lv_obj_t *sl = lv_bar_create(br);
    lv_obj_set_size(sl, 96, 6);
    lv_obj_set_style_bg_color(sl, ui_col(0x22262d), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sl, ui_col(UI_C_BLUE), LV_PART_INDICATOR);
    lv_bar_set_value(sl, 80, LV_ANIM_OFF);
    ui_label(br, "80%", UI_FONT_SM, UI_C_TEXT_2);

    /* footer */
    lv_obj_t *foot = ui_box(ui_generalSimpleScreen);
    lv_obj_set_size(foot, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(foot, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(foot, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(foot, 6, 0);
    ui_icon(foot, UI_SYM_DEVICE_MOBILE_MESSAGE, UI_ICON_SM, UI_C_TEXT_MUTED);
    ui_label(foot, "Red, nube y nombre del equipo se gestionan desde la app por BLE",
             UI_FONT_XS, UI_C_TEXT_MUTED);
}

void ui_generalSimpleScreen_screen_destroy(void)
{
    if (ui_generalSimpleScreen) { lv_obj_del(ui_generalSimpleScreen); ui_generalSimpleScreen = NULL; }
}

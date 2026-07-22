#include "ui_splashScreen.h"
#include "ui_widgets.h"
#include "ui_theme.h"

/* Splash de arranque (mockup 4a). */

lv_obj_t *ui_splashScreen = NULL;

static lv_obj_t *status_chip(lv_obj_t *parent, const char *sym, const char *txt, uint32_t icon_col)
{
    lv_obj_t *c = ui_box(parent);
    lv_obj_set_size(c, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(c, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(c, 5, 0);
    ui_icon(c, sym, UI_ICON_SM, icon_col);
    ui_label(c, txt, UI_FONT_XS, UI_C_TEXT_MUTED);
    return c;
}

void ui_splashScreen_screen_init(void)
{
    ui_splashScreen = ui_screen_base();
    lv_obj_set_flex_flow(ui_splashScreen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui_splashScreen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(ui_splashScreen, 18, 0);

    /* icono de marca 64x64 */
    lv_obj_t *badge = ui_icon_badge(ui_splashScreen, UI_SYM_ACTIVITY, UI_ICON_LG, 0x9fe1cb, UI_C_HEADER_ICONBG, 64);
    lv_obj_set_style_radius(badge, 18, 0);

    lv_obj_t *title = ui_label(ui_splashScreen, "Axira Equipos", UI_FONT_XL, UI_C_TEXT);
    lv_obj_set_style_margin_top(title, 14, 0);
    lv_obj_t *sub = ui_label(ui_splashScreen, "Medidor de presión y flujo de gases medicinales",
                             UI_FONT_SM, UI_C_TEXT_3);
    lv_obj_set_style_margin_top(sub, 2, 0);

    /* barra de progreso */
    lv_obj_t *bar = lv_bar_create(ui_splashScreen);
    lv_obj_set_size(bar, 250, 5);
    lv_obj_set_style_margin_top(bar, 24, 0);
    lv_obj_set_style_radius(bar, 3, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, ui_col(0x22262d), LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 3, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar, ui_col(UI_C_OK), LV_PART_INDICATOR);
    lv_bar_set_value(bar, 72, LV_ANIM_OFF);

    /* línea "inicializando" */
    lv_obj_t *ini = status_chip(ui_splashScreen, UI_SYM_LOADER_2,
                                "Inicializando MCU · autotest de sensores…", UI_C_OK_DIM);
    lv_obj_set_style_margin_top(ini, 10, 0);
    lv_obj_set_style_text_color(lv_obj_get_child(ini, 1), ui_col(UI_C_OK_DIM), 0);

    /* fila de chips de autotest */
    lv_obj_t *chips = ui_box(ui_splashScreen);
    lv_obj_set_size(chips, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(chips, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(chips, 16, 0);
    lv_obj_set_style_margin_top(chips, 18, 0);
    status_chip(chips, UI_SYM_CIRCLE_CHECK_FILLED, "MCU control OK", UI_C_OK);
    status_chip(chips, UI_SYM_CIRCLE_CHECK_FILLED, "Bus SPI TFT", UI_C_OK);
    status_chip(chips, UI_SYM_LOADER_2, "Sensores", UI_C_WARN_SOFT);

    /* versión al pie */
    lv_obj_t *ver = ui_label(ui_splashScreen, "FW ctrl v2.4.1 · FW app v1.8.0 · NFPA 99",
                             UI_FONT_XS, 0x4a5058);
    lv_obj_add_flag(ver, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_align(ver, LV_ALIGN_BOTTOM_MID, 0, -4);
}

void ui_splashScreen_screen_destroy(void)
{
    if (ui_splashScreen) { lv_obj_del(ui_splashScreen); ui_splashScreen = NULL; }
}

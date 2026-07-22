#include "ui_confirmScreen.h"
#include "ui_i18n.h"
#include "ui_widgets.h"
#include "ui_theme.h"
#include "ui.h"
#include "ui_cfg.h"
#include <stdio.h>

/* Diálogo de confirmación de cambio crítico (mockup 5d). */

lv_obj_t *ui_confirmScreen = NULL;

void ui_confirmScreen_screen_init(void)
{
    ui_confirmScreen = ui_screen_base();
    lv_obj_set_style_bg_color(ui_confirmScreen, ui_col(0x08090b), 0);  /* scrim oscuro */
    lv_obj_set_style_pad_all(ui_confirmScreen, 0, 0);
    lv_obj_set_flex_flow(ui_confirmScreen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui_confirmScreen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *card = lv_obj_create(ui_confirmScreen);
    ui_kill_scroll(card);
    lv_obj_set_width(card, 340);
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, ui_col(0x1b1e24), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, UI_RADIUS_CARD, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, ui_col(0x33373f), 0);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(card, 9, 0);

    /* título */
    lv_obj_t *tt = ui_box(card);
    lv_obj_set_size(tt, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(tt, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tt, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(tt, 9, 0);
    lv_obj_t *ib = lv_obj_create(tt);
    ui_kill_scroll(ib);
    lv_obj_set_size(ib, 32, 32);
    lv_obj_set_style_radius(ib, UI_RADIUS_SM, 0);
    lv_obj_set_style_bg_color(ib, ui_col(UI_C_WARN_SOFT), 0);
    lv_obj_set_style_bg_opa(ib, LV_OPA_20, 0);
    lv_obj_center(ui_icon(ib, UI_SYM_ALERT_TRIANGLE, UI_ICON_MD, UI_C_WARN_SOFT));
    ui_label(tt, _t("Confirmar cambio crítico"), UI_FONT_TITLE, UI_C_TEXT);

    lv_obj_t *desc = ui_label(card, _t("Vas a modificar un umbral de alarma de seguridad. Se enviará al "
                              "MCU de control y se registrará con fecha y hora."), UI_FONT_SM, UI_C_TEXT_3);
    lv_obj_set_width(desc, LV_PCT(100));
    lv_label_set_long_mode(desc, LV_LABEL_LONG_WRAP);

    /* fila antes -> después */
    lv_obj_t *row = ui_box(card);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(row, ui_col(0x12181f), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(row, UI_RADIUS_SM, 0);
    lv_obj_set_style_pad_hor(row, 13, 0);
    lv_obj_set_style_pad_ver(row, 10, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 8, 0);
    lv_obj_t *lc = ui_box(row);
    lv_obj_set_flex_grow(lc, 1);
    lv_obj_set_height(lc, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(lc, LV_FLEX_FLOW_COLUMN);
    lv_obj_t *cap = ui_label(lc, ui_edit_label(), UI_FONT_XS, UI_C_TEXT_MUTED);
    lv_obj_set_style_text_letter_space(cap, 1, 0);
    ui_label(lc, _t("antes"), UI_FONT_SM, UI_C_TEXT_2);
    char ob[16], nb[16];
    ui_cfg_press_fmt(ob, sizeof(ob), ui_edit_old());
    ui_cfg_press_fmt(nb, sizeof(nb), ui_edit_new());
    ui_label(row, ob, UI_FONT_LG, UI_C_TEXT_2);
    ui_icon(row, UI_SYM_ARROW_RIGHT, UI_ICON_SM, UI_C_TEXT_MUTED);
    ui_label(row, nb, UI_FONT_LG, UI_C_OK);
    ui_label(row, ui_edit_unit(), UI_FONT_XS, UI_C_TEXT_2);

    /* botones */
    lv_obj_t *btns = ui_box(card);
    lv_obj_set_size(btns, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(btns, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(btns, 8, 0);
    lv_obj_t *cancel = ui_card(btns);
    lv_obj_set_flex_grow(cancel, 1);
    lv_obj_set_style_radius(cancel, UI_RADIUS_PILL, 0);
    lv_obj_set_style_pad_ver(cancel, 10, 0);
    lv_obj_add_flag(cancel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(cancel, ui_nav_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_center(ui_label(cancel, _t("Cancelar"), UI_FONT_MD, 0xcfd3d9));
    lv_obj_t *ok = ui_box(btns);
    lv_obj_set_flex_grow(ok, 2);
    ui_style_button(ok, UI_C_OK);
    lv_obj_set_style_pad_ver(ok, 10, 0);
    lv_obj_set_flex_flow(ok, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ok, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(ok, 7, 0);
    lv_obj_add_flag(ok, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ok, ui_open_pin_cb, LV_EVENT_CLICKED, NULL);
    ui_icon(ok, UI_SYM_SHIELD_CHECK, UI_ICON_SM, 0x06251f);
    ui_label(ok, _t("Confirmar con PIN"), UI_FONT_MD, 0x06251f);
}

void ui_confirmScreen_screen_destroy(void)
{
    if (ui_confirmScreen) { lv_obj_del(ui_confirmScreen); ui_confirmScreen = NULL; }
}

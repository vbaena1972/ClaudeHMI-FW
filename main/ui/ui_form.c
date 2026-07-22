#include "ui_form.h"
#include "ui_i18n.h"
#include "ui_widgets.h"
#include "ui_theme.h"
#include "ui_nav.h"

/* Teclado compartido de la pantalla de formulario activa. */
static lv_obj_t *s_kb = NULL;

static void ta_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    if (!s_kb) return;

    if (code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(s_kb, ta);
        lv_obj_clear_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_kb);
    } else if (code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(s_kb, NULL);
        lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    }
}

static void kb_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t *ui_form_begin(const char *title, lv_obj_t **out_content, lv_event_cb_t save_cb)
{
    lv_obj_t *scr = ui_screen_base();
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(scr, 8, 0);
    lv_obj_set_style_pad_row(scr, 6, 0);

    /* header con back + título + Guardar */
    lv_obj_t *hdr = ui_nav_header(scr, title);
    lv_obj_t *save = ui_box(hdr);
    lv_obj_set_size(save, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    ui_style_button(save, UI_C_OK);
    lv_obj_set_style_pad_hor(save, 14, 0);
    lv_obj_set_style_pad_ver(save, 6, 0);
    lv_obj_add_flag(save, LV_OBJ_FLAG_CLICKABLE);
    if (save_cb) lv_obj_add_event_cb(save, save_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_center(ui_label(save, _t("Guardar"), UI_FONT_SM, 0x06251f));

    /* contenido con scroll vertical */
    lv_obj_t *content = ui_box(scr);
    lv_obj_set_width(content, LV_PCT(100));
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, 7, 0);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    /* teclado (oculto hasta enfocar un campo) */
    s_kb = lv_keyboard_create(scr);
    lv_obj_set_size(s_kb, LV_PCT(100), 150);
    lv_obj_align(s_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_kb, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_event_cb(s_kb, kb_event_cb, LV_EVENT_ALL, NULL);

    if (out_content) *out_content = content;
    return scr;
}

lv_obj_t *ui_form_textarea(lv_obj_t *parent, const char *caption, const char *value, bool password)
{
    lv_obj_t *c = ui_card(parent);
    lv_obj_set_width(c, LV_PCT(100));
    lv_obj_set_style_radius(c, 10, 0);
    lv_obj_set_style_pad_hor(c, 11, 0);
    lv_obj_set_style_pad_ver(c, 7, 0);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(c, 4, 0);
    lv_obj_t *l = ui_label(c, caption, UI_FONT_XS, UI_C_TEXT_MUTED);
    lv_obj_set_style_text_letter_space(l, 1, 0);

    lv_obj_t *ta = lv_textarea_create(c);
    lv_obj_set_width(ta, LV_PCT(100));
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_password_mode(ta, password);
    lv_textarea_set_text(ta, value ? value : "");
    lv_obj_set_style_bg_color(ta, ui_col(0x12151a), 0);
    lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ta, 1, 0);
    lv_obj_set_style_border_color(ta, ui_col(UI_C_BORDER), 0);
    lv_obj_set_style_radius(ta, UI_RADIUS_SM, 0);
    lv_obj_set_style_text_color(ta, ui_col(UI_C_TEXT), 0);
    lv_obj_set_style_text_font(ta, UI_FONT_MD, 0);
    lv_obj_set_style_pad_all(ta, 6, 0);
    lv_obj_add_event_cb(ta, ta_event_cb, LV_EVENT_ALL, NULL);
    return ta;
}

lv_obj_t *ui_form_switch(lv_obj_t *parent, const char *label, const char *sub, bool on)
{
    lv_obj_t *row = ui_card(parent);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_style_radius(row, 10, 0);
    lv_obj_set_style_pad_hor(row, 12, 0);
    lv_obj_set_style_pad_ver(row, 8, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *col = ui_box(row);
    lv_obj_set_flex_grow(col, 1);
    lv_obj_set_height(col, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    ui_label(col, label, UI_FONT_MD, UI_C_TEXT);
    if (sub) ui_label(col, sub, UI_FONT_XS, UI_C_TEXT_MUTED);

    lv_obj_t *sw = lv_switch_create(row);
    lv_obj_set_style_bg_color(sw, ui_col(UI_C_OK), LV_PART_INDICATOR | LV_STATE_CHECKED);
    if (on) lv_obj_add_state(sw, LV_STATE_CHECKED);
    return sw;
}

lv_obj_t *ui_form_dropdown(lv_obj_t *parent, const char *caption, const char *opts, int selected)
{
    lv_obj_t *c = ui_card(parent);
    lv_obj_set_width(c, LV_PCT(100));
    lv_obj_set_style_radius(c, 10, 0);
    lv_obj_set_style_pad_hor(c, 11, 0);
    lv_obj_set_style_pad_ver(c, 7, 0);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(c, 4, 0);
    lv_obj_t *l = ui_label(c, caption, UI_FONT_XS, UI_C_TEXT_MUTED);
    lv_obj_set_style_text_letter_space(l, 1, 0);

    lv_obj_t *dd = lv_dropdown_create(c);
    lv_obj_set_width(dd, LV_PCT(100));
    lv_dropdown_set_options(dd, opts);
    if (selected >= 0) lv_dropdown_set_selected(dd, selected);
    lv_obj_set_style_bg_color(dd, ui_col(0x12151a), 0);
    lv_obj_set_style_border_color(dd, ui_col(UI_C_BORDER), 0);
    lv_obj_set_style_radius(dd, UI_RADIUS_SM, 0);
    lv_obj_set_style_text_color(dd, ui_col(UI_C_TEXT), 0);
    lv_obj_set_style_text_font(dd, UI_FONT_MD, 0);
    return dd;
}

lv_obj_t *ui_form_row2(lv_obj_t *parent)
{
    lv_obj_t *row = ui_box(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, 7, 0);
    return row;
}

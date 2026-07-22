#include "ui_widgets.h"
#include "ui_nav.h"

lv_obj_t *ui_nav_header(lv_obj_t *parent, const char *title)
{
    lv_obj_t *h = ui_box(parent);
    lv_obj_set_size(h, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(h, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(h, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *left = ui_box(h);
    lv_obj_set_size(left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(left, 9, 0);

    lv_obj_t *back = lv_obj_create(left);
    ui_kill_scroll(back);
    lv_obj_set_size(back, 28, 28);
    lv_obj_set_style_radius(back, 7, 0);
    lv_obj_set_style_bg_opa(back, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(back, 1, 0);
    lv_obj_set_style_border_color(back, ui_col(0x3a3f47), 0);
    lv_obj_add_flag(back, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(back, ui_nav_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bi = ui_icon(back, UI_SYM_ARROW_LEFT, UI_ICON_SM, 0xcfd3d9);
    lv_obj_center(bi);

    ui_label(left, title, UI_FONT_TITLE, UI_C_TEXT);
    return h;
}

void ui_kill_scroll(lv_obj_t *obj)
{
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
}

lv_obj_t *ui_screen_base(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, ui_col(UI_C_SCREEN_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(scr, ui_col(UI_C_TEXT), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(scr, UI_FONT_MD, LV_PART_MAIN | LV_STATE_DEFAULT);
    ui_kill_scroll(scr);
    lv_obj_set_style_pad_all(scr, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    return scr;
}

lv_obj_t *ui_box(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    ui_kill_scroll(o);
    /* En LVGL 9 lv_obj_create es CLICKABLE por defecto: un contenedor de layout
       no debe robar el toque de una tarjeta/tile clickeable que lo envuelve. */
    lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
    return o;
}

lv_obj_t *ui_card(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_style_bg_color(o, ui_col(UI_C_CARD_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(o, UI_RADIUS_CARD, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(o, ui_col(UI_C_BORDER), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(o, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(o, LV_OPA_60, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(o, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

lv_obj_t *ui_label(lv_obj_t *parent, const char *txt, const lv_font_t *font, uint32_t color_hex)
{
    lv_obj_t *l = lv_label_create(parent);
    if (txt) lv_label_set_text(l, txt);
    if (font) lv_obj_set_style_text_font(l, font, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(l, ui_col(color_hex), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(l, LV_OBJ_FLAG_CLICKABLE);  /* los labels no roban el toque */
    return l;
}

lv_obj_t *ui_icon(lv_obj_t *parent, const char *sym, const lv_font_t *font, uint32_t color_hex)
{
    return ui_label(parent, sym, font ? font : UI_ICON_SM, color_hex);
}

lv_obj_t *ui_pill(lv_obj_t *parent, const char *txt, const lv_font_t *font,
                  uint32_t text_hex, uint32_t bg_hex, uint32_t border_hex)
{
    lv_obj_t *p = lv_obj_create(parent);
    ui_kill_scroll(p);
    lv_obj_set_style_bg_color(p, ui_col(bg_hex), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(p, UI_RADIUS_SM, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(p, ui_col(border_hex), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(p, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_hor(p, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_ver(p, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(p, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_t *l = ui_label(p, txt, font, text_hex);
    lv_obj_center(l);
    return p;
}

lv_obj_t *ui_icon_badge(lv_obj_t *parent, const char *sym, const lv_font_t *font,
                        uint32_t icon_hex, uint32_t bg_hex, lv_coord_t size)
{
    lv_obj_t *b = lv_obj_create(parent);
    ui_kill_scroll(b);
    lv_obj_clear_flag(b, LV_OBJ_FLAG_CLICKABLE);  /* decorativo por defecto; back/gear añaden el flag */
    lv_obj_set_size(b, size, size);
    lv_obj_set_style_bg_color(b, ui_col(bg_hex), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(b, UI_RADIUS_SM, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *ic = ui_icon(b, sym, font, icon_hex);
    lv_obj_center(ic);
    return b;
}

void ui_style_button(lv_obj_t *obj, uint32_t bg_hex)
{
    lv_obj_set_style_bg_color(obj, ui_col(bg_hex), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj, UI_RADIUS_PILL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(obj, ui_col(bg_hex), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(obj, LV_OPA_80, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
}

lv_obj_t *ui_header_back(lv_obj_t *parent, const char *title, const char *subtitle,
                         lv_obj_t **out_back)
{
    lv_obj_t *h = ui_box(parent);
    lv_obj_set_width(h, LV_PCT(100));
    lv_obj_set_height(h, 40);
    lv_obj_set_flex_flow(h, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(h, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(h, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *back = ui_icon_badge(h, UI_SYM_ARROW_LEFT, UI_ICON_MD, UI_C_TEXT, UI_C_CARD_BG, 32);
    lv_obj_add_flag(back, LV_OBJ_FLAG_CLICKABLE);
    if (out_back) *out_back = back;

    lv_obj_t *col = ui_box(h);
    lv_obj_set_size(col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(col, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    ui_label(col, title, UI_FONT_TITLE, UI_C_TEXT);
    if (subtitle) ui_label(col, subtitle, UI_FONT_SM, UI_C_TEXT_3);

    return h;
}

lv_obj_t *ui_menu_row(lv_obj_t *parent, const char *sym, const char *title,
                      const char *subtitle, const char *value, lv_obj_t **out_value)
{
    lv_obj_t *row = ui_card(parent);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(row, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(row, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);

    ui_icon_badge(row, sym, UI_ICON_MD, UI_C_OK, UI_C_HEADER_ICONBG, 34);

    lv_obj_t *col = ui_box(row);
    lv_obj_set_flex_grow(col, 1);
    lv_obj_set_height(col, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(col, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    ui_label(col, title, UI_FONT_LG, UI_C_TEXT);
    if (subtitle) ui_label(col, subtitle, UI_FONT_SM, UI_C_TEXT_3);

    if (value) {
        lv_obj_t *v = ui_label(row, value, UI_FONT_MD, UI_C_TEXT_2);
        if (out_value) *out_value = v;
    } else if (out_value) {
        *out_value = NULL;
    }
    ui_icon(row, UI_SYM_CHEVRON_RIGHT, UI_ICON_SM, UI_C_TEXT_MUTED);

    return row;
}

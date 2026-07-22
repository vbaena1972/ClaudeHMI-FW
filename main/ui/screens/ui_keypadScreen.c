#include "ui_keypadScreen.h"
#include "ui_widgets.h"
#include "ui_theme.h"
#include "ui.h"
#include "ui_cfg.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Teclado numérico de umbral (mockup 5c). */

lv_obj_t *ui_keypadScreen = NULL;
static lv_obj_t *s_value_lbl = NULL;
static char s_buf[8] = "0";

/* Aceptar: guarda el valor tecleado en la edición pendiente y abre el diálogo de confirmación. */
static void accept_cb(lv_event_t *e)
{
    ui_edit_set_new((float)atof(s_buf[0] ? s_buf : "0"));
    ui_open_confirm_cb(e);
}

static void refresh_value(void)
{
    if (s_value_lbl) lv_label_set_text(s_value_lbl, s_buf[0] ? s_buf : "0");
}

static void key_cb(lv_event_t *e)
{
    char k = (char)(intptr_t)lv_event_get_user_data(e);
    size_t len = strlen(s_buf);
    if (k == 'C') { s_buf[0] = '\0'; }
    else if (k == '<') { if (len) s_buf[len - 1] = '\0'; }
    else if (k >= '0' && k <= '9') {
        if (len < sizeof(s_buf) - 1) { s_buf[len] = k; s_buf[len + 1] = '\0'; }
    }
    refresh_value();
}

static lv_obj_t *key(lv_obj_t *grid, const char *txt, const char *sym, char code,
                     int col, int row, uint32_t txt_col)
{
    lv_obj_t *k = ui_card(grid);
    lv_obj_set_grid_cell(k, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_set_style_radius(k, UI_RADIUS_PILL, 0);
    lv_obj_add_flag(k, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(k, key_cb, LV_EVENT_CLICKED, (void *)(intptr_t)code);
    lv_obj_t *l = sym ? ui_icon(k, sym, UI_ICON_MD, txt_col) : ui_label(k, txt, UI_FONT_XL, txt_col);
    lv_obj_center(l);
    return k;
}

static int32_t s_kcol[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
static int32_t s_krow[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

void ui_keypadScreen_screen_init(void)
{
    snprintf(s_buf, sizeof(s_buf), "%.0f", ui_edit_new());  /* valor actual del umbral */
    ui_keypadScreen = ui_screen_base();
    lv_obj_set_flex_flow(ui_keypadScreen, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(ui_keypadScreen, 12, 0);
    lv_obj_set_style_pad_column(ui_keypadScreen, 16, 0);

    /* --- columna izquierda --- */
    lv_obj_t *left = ui_box(ui_keypadScreen);
    lv_obj_set_flex_grow(left, 1);
    lv_obj_set_height(left, LV_PCT(100));
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(left, 9, 0);

    lv_obj_t *tt = ui_box(left);
    lv_obj_set_size(tt, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(tt, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tt, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(tt, 8, 0);
    ui_icon(tt, UI_SYM_GAUGE, UI_ICON_MD, UI_C_ALARM);
    ui_label(tt, ui_edit_label(), UI_FONT_MD, UI_C_TEXT);

    lv_obj_t *disp = ui_box(left);
    lv_obj_set_size(disp, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(disp, ui_col(0x12181f), 0);
    lv_obj_set_style_bg_opa(disp, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(disp, UI_RADIUS_CARD, 0);
    lv_obj_set_style_border_width(disp, 1, 0);
    lv_obj_set_style_border_color(disp, ui_col(UI_C_ALARM), 0);
    lv_obj_set_style_border_opa(disp, LV_OPA_40, 0);
    lv_obj_set_style_pad_ver(disp, 12, 0);
    lv_obj_set_flex_flow(disp, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(disp, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(disp, 8, 0);
    s_value_lbl = ui_label(disp, s_buf, UI_FONT_HUGE, UI_C_TEXT_STRONG);
    ui_label(disp, ui_edit_unit(), UI_FONT_LG, UI_C_TEXT_2);

    lv_obj_t *info = ui_box(left);
    lv_obj_set_size(info, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(info, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(info, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    ui_label(info, "rango sensor 0–2200", UI_FONT_XS, UI_C_TEXT_MUTED);
    ui_label(info, "✓ válido", UI_FONT_XS, UI_C_OK);

    lv_obj_t *sp = ui_box(left); lv_obj_set_flex_grow(sp, 1); lv_obj_set_width(sp, LV_PCT(100));

    lv_obj_t *btns = ui_box(left);
    lv_obj_set_size(btns, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(btns, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(btns, 8, 0);
    lv_obj_t *cancel = ui_card(btns);
    lv_obj_set_flex_grow(cancel, 1);
    lv_obj_set_style_radius(cancel, UI_RADIUS_PILL, 0);
    lv_obj_set_style_pad_ver(cancel, 9, 0);
    lv_obj_add_flag(cancel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(cancel, ui_nav_back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_center(ui_label(cancel, "Cancelar", UI_FONT_SM, 0xcfd3d9));
    lv_obj_t *ok = ui_box(btns);
    lv_obj_set_flex_grow(ok, 1);
    ui_style_button(ok, UI_C_OK);
    lv_obj_set_style_pad_ver(ok, 9, 0);
    lv_obj_add_flag(ok, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ok, accept_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_center(ui_label(ok, "Aceptar", UI_FONT_SM, 0x06251f));

    /* --- teclado numérico --- */
    lv_obj_t *pad = ui_box(ui_keypadScreen);
    lv_obj_set_size(pad, 200, LV_PCT(100));
    lv_obj_set_style_pad_column(pad, 6, 0);
    lv_obj_set_style_pad_row(pad, 6, 0);
    lv_obj_set_grid_dsc_array(pad, s_kcol, s_krow);
    lv_obj_set_layout(pad, LV_LAYOUT_GRID);
    const char *digits[9] = { "1","2","3","4","5","6","7","8","9" };
    for (int i = 0; i < 9; i++)
        key(pad, digits[i], NULL, (char)('1' + i), i % 3, i / 3, UI_C_TEXT);
    key(pad, "C", NULL, 'C', 0, 3, UI_C_TEXT_2);
    key(pad, "0", NULL, '0', 1, 3, UI_C_TEXT);
    key(pad, NULL, UI_SYM_BACKSPACE, '<', 2, 3, 0xcfd3d9);

    refresh_value();
}

void ui_keypadScreen_screen_destroy(void)
{
    if (ui_keypadScreen) { lv_obj_del(ui_keypadScreen); ui_keypadScreen = NULL; s_value_lbl = NULL; }
}

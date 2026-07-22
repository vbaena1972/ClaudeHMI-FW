#include "ui_pinScreen.h"
#include "ui_i18n.h"
#include "ui_widgets.h"
#include "ui_theme.h"
#include "ui.h"
#include "ui_cfg.h"
#include <string.h>

/* Diálogo de PIN de servicio (mockup 5a). */

lv_obj_t *ui_pinScreen = NULL;
static lv_obj_t *s_dots[4];
static char s_pin[5];
static int s_count = 0;

static void refresh_dots(void)
{
    for (int i = 0; i < 4; i++) {
        bool on = (i < s_count);
        lv_obj_set_style_bg_color(s_dots[i], ui_col(UI_C_OK), 0);
        lv_obj_set_style_bg_opa(s_dots[i], on ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_opa(s_dots[i], on ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    }
}

static void pin_wrong_flash(void)
{
    /* pinta los puntos en rojo brevemente y limpia */
    for (int i = 0; i < 4; i++) {
        lv_obj_set_style_bg_color(s_dots[i], ui_col(UI_C_ALARM), 0);
        lv_obj_set_style_bg_opa(s_dots[i], LV_OPA_COVER, 0);
    }
}

static void pin_key_cb(lv_event_t *e)
{
    char k = (char)(intptr_t)lv_event_get_user_data(e);
    if (k == '<') {
        if (s_count > 0) s_pin[--s_count] = '\0';
    } else if (k == '=') {
        if (s_count == 4 && ui_cfg_check_pin(s_pin)) {
            ui_edit_apply();                       /* aplica el umbral en NVS */
            ui_nav_pop_to(ui_sensorEditScreen);    /* vuelve a Sensores       */
            return;
        }
        pin_wrong_flash();                          /* PIN incorrecto o incompleto */
        s_count = 0; s_pin[0] = '\0';
        return;
    } else if (s_count < 4 && k >= '0' && k <= '9') {
        s_pin[s_count++] = k; s_pin[s_count] = '\0';
    }
    refresh_dots();
}

static lv_obj_t *pkey(lv_obj_t *grid, const char *txt, const char *sym, char code, int col, int row,
                      uint32_t bg, uint32_t txt_col, bool bordered)
{
    lv_obj_t *k = ui_box(grid);
    lv_obj_set_grid_cell(k, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_set_style_radius(k, 10, 0);
    if (bg) { lv_obj_set_style_bg_color(k, ui_col(bg), 0); lv_obj_set_style_bg_opa(k, LV_OPA_COVER, 0); }
    if (bordered) {
        lv_obj_set_style_border_width(k, 1, 0);
        lv_obj_set_style_border_color(k, ui_col(0x2c313a), 0);
    }
    lv_obj_add_flag(k, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(k, pin_key_cb, LV_EVENT_CLICKED, (void *)(intptr_t)code);
    lv_obj_t *l = sym ? ui_icon(k, sym, UI_ICON_MD, txt_col) : ui_label(k, txt, UI_FONT_XL, txt_col);
    lv_obj_center(l);
    return k;
}

static int32_t s_col[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
static int32_t s_row[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

void ui_pinScreen_screen_init(void)
{
    s_count = 0; s_pin[0] = '\0';
    ui_pinScreen = ui_screen_base();
    lv_obj_set_flex_flow(ui_pinScreen, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(ui_pinScreen, 14, 0);
    lv_obj_set_style_pad_column(ui_pinScreen, 18, 0);

    /* izquierda */
    lv_obj_t *left = ui_box(ui_pinScreen);
    lv_obj_set_flex_grow(left, 1);
    lv_obj_set_height(left, LV_PCT(100));
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *badge = lv_obj_create(left);
    ui_kill_scroll(badge);
    lv_obj_set_size(badge, 38, 38);
    lv_obj_set_style_radius(badge, 10, 0);
    lv_obj_set_style_bg_color(badge, ui_col(UI_C_OK), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_20, 0);
    lv_obj_center(ui_icon(badge, UI_SYM_LOCK, UI_ICON_MD, UI_C_OK));

    lv_obj_t *t = ui_label(left, _t("Acceso protegido"), UI_FONT_XL, UI_C_TEXT);
    lv_obj_set_style_margin_top(t, 12, 0);
    lv_obj_t *desc = ui_label(left, _t("Introduce el PIN de servicio para cambiar parámetros críticos."),
                              UI_FONT_SM, UI_C_TEXT_3);
    lv_obj_set_width(desc, LV_PCT(100));
    lv_label_set_long_mode(desc, LV_LABEL_LONG_WRAP);

    lv_obj_t *dots = ui_box(left);
    lv_obj_set_size(dots, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(dots, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(dots, 11, 0);
    lv_obj_set_style_margin_top(dots, 18, 0);
    for (int i = 0; i < 4; i++) {
        lv_obj_t *d = ui_box(dots);
        lv_obj_set_size(d, 16, 16);
        lv_obj_set_style_radius(d, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(d, ui_col(UI_C_OK), 0);
        lv_obj_set_style_border_width(d, 2, 0);
        lv_obj_set_style_border_color(d, ui_col(0x3a3f47), 0);
        s_dots[i] = d;
    }

    lv_obj_t *note = ui_box(left);
    lv_obj_set_size(note, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(note, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(note, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(note, 6, 0);
    lv_obj_set_style_margin_top(note, 16, 0);
    ui_icon(note, UI_SYM_SHIELD_LOCK, UI_ICON_SM, UI_C_TEXT_MUTED);
    ui_label(note, _t("Cambios se registran con RTC y usuario"), UI_FONT_XS, UI_C_TEXT_MUTED);

    /* teclado */
    lv_obj_t *pad = ui_box(ui_pinScreen);
    lv_obj_set_size(pad, 210, LV_PCT(100));
    lv_obj_set_style_pad_column(pad, 7, 0);
    lv_obj_set_style_pad_row(pad, 7, 0);
    lv_obj_set_grid_dsc_array(pad, s_col, s_row);
    lv_obj_set_layout(pad, LV_LAYOUT_GRID);
    const char *digits[9] = { "1","2","3","4","5","6","7","8","9" };
    for (int i = 0; i < 9; i++)
        pkey(pad, digits[i], NULL, (char)('1' + i), i % 3, i / 3, UI_C_CARD_BG, UI_C_TEXT, true);
    pkey(pad, NULL, UI_SYM_ARROW_BACK_UP, '<', 0, 3, 0, UI_C_TEXT_MUTED, false);
    pkey(pad, "0", NULL, '0', 1, 3, UI_C_CARD_BG, UI_C_TEXT, true);
    pkey(pad, NULL, UI_SYM_CHECK, '=', 2, 3, UI_C_OK_BG, UI_C_OK_SOFT, false);

    refresh_dots();
}

void ui_pinScreen_screen_destroy(void)
{
    if (ui_pinScreen) { lv_obj_del(ui_pinScreen); ui_pinScreen = NULL; }
}

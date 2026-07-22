#include "ui_bleAppScreen.h"
#include "ui_widgets.h"
#include "ui_theme.h"
#include "ui_nav.h"

/* Configurar por app (BLE) (mockup 5f). */

lv_obj_t *ui_bleAppScreen = NULL;

void ui_bleAppScreen_screen_init(void)
{
    ui_bleAppScreen = ui_screen_base();
    lv_obj_set_flex_flow(ui_bleAppScreen, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(ui_bleAppScreen, 16, 0);
    lv_obj_set_style_pad_column(ui_bleAppScreen, 20, 0);
    lv_obj_set_flex_align(ui_bleAppScreen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* back flotante */
    lv_obj_t *back = ui_icon_badge(ui_bleAppScreen, UI_SYM_ARROW_LEFT, UI_ICON_SM, 0xcfd3d9, UI_C_CARD_BG, 28);
    lv_obj_add_flag(back, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_align(back, LV_ALIGN_TOP_LEFT, 4, 4);
    lv_obj_add_flag(back, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(back, ui_nav_back_event_cb, LV_EVENT_CLICKED, NULL);

    /* izquierda */
    lv_obj_t *left = ui_box(ui_bleAppScreen);
    lv_obj_set_flex_grow(left, 1);
    lv_obj_set_height(left, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *badge = lv_obj_create(left);
    ui_kill_scroll(badge);
    lv_obj_set_size(badge, 38, 38);
    lv_obj_set_style_radius(badge, 10, 0);
    lv_obj_set_style_bg_color(badge, ui_col(UI_C_BLUE), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_20, 0);
    lv_obj_center(ui_icon(badge, UI_SYM_BLUETOOTH, UI_ICON_MD, UI_C_BLUE));

    lv_obj_t *t = ui_label(left, "Configurar por app", UI_FONT_XL, UI_C_TEXT);
    lv_obj_set_style_margin_top(t, 12, 0);
    lv_obj_t *desc = ui_label(left, "Wi-Fi, contraseñas, certificados AWS, IP y nombre del equipo se "
                              "escriben desde la app de servicio — no se teclean en la pantalla.",
                              UI_FONT_SM, UI_C_TEXT_3);
    lv_obj_set_width(desc, LV_PCT(100));
    lv_label_set_long_mode(desc, LV_LABEL_LONG_WRAP);

    lv_obj_t *adv = ui_box(left);
    lv_obj_set_size(adv, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(adv, ui_col(UI_C_OK_BG), 0);
    lv_obj_set_style_bg_opa(adv, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(adv, UI_RADIUS_SM, 0);
    lv_obj_set_style_border_width(adv, 1, 0);
    lv_obj_set_style_border_color(adv, ui_col(UI_C_OK_BORDER), 0);
    lv_obj_set_style_pad_hor(adv, 12, 0);
    lv_obj_set_style_pad_ver(adv, 8, 0);
    lv_obj_set_style_margin_top(adv, 16, 0);
    lv_obj_set_flex_flow(adv, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(adv, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(adv, 8, 0);
    ui_icon(adv, UI_SYM_RADAR_2, UI_ICON_SM, UI_C_OK_SOFT);
    ui_label(adv, "Anunciando por BLE · visible como MPF-4200-1847", UI_FONT_SM, UI_C_OK_SOFT);

    lv_obj_t *dl = ui_label(left, "Descarga «Axira Service» · empareja y continúa en el teléfono",
                            UI_FONT_XS, UI_C_TEXT_MUTED);
    lv_obj_set_style_margin_top(dl, 10, 0);

    /* derecha: QR */
    lv_obj_t *right = ui_box(ui_bleAppScreen);
    lv_obj_set_size(right, 132, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(right, 9, 0);
    lv_obj_t *qr = ui_box(right);
    lv_obj_set_size(qr, 118, 118);
    lv_obj_set_style_bg_color(qr, ui_col(0xffffff), 0);
    lv_obj_set_style_bg_opa(qr, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(qr, UI_RADIUS_TILE, 0);
    lv_obj_center(ui_icon(qr, UI_SYM_QRCODE, UI_ICON_LG, 0x0f1115));
    ui_label(right, "Escanea para emparejar", UI_FONT_XS, UI_C_TEXT_3);
}

void ui_bleAppScreen_screen_destroy(void)
{
    if (ui_bleAppScreen) { lv_obj_del(ui_bleAppScreen); ui_bleAppScreen = NULL; }
}

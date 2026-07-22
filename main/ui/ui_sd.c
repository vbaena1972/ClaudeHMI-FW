#include "ui_sd.h"
#include "ui_i18n.h"
#include "ui_widgets.h"
#include "ui_theme.h"

/* Overlay modal de mantenimiento por microSD.
 * Reemplaza la vieja create_sd_update_ui() de SquareLine (que quedó comentada
 * en main.c y dejaba los widgets NULL -> crash al insertar la SD). */

static lv_obj_t *s_overlay = NULL;   /* velo a pantalla completa en layer_top */
static lv_obj_t *s_bar = NULL;
static lv_obj_t *s_status = NULL;
static lv_obj_t *s_btn = NULL;

void ui_sd_show(void)
{
    if (s_overlay) return;

    /* velo oscuro que bloquea el táctil de la pantalla de abajo */
    s_overlay = lv_obj_create(lv_layer_top());
    ui_kill_scroll(s_overlay);
    lv_obj_set_size(s_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_overlay, ui_col(0x000000), 0);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_60, 0);
    lv_obj_set_style_border_width(s_overlay, 0, 0);
    lv_obj_set_style_radius(s_overlay, 0, 0);
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_CLICKABLE);  /* absorbe toques */

    /* tarjeta central */
    lv_obj_t *card = ui_card(s_overlay);
    lv_obj_set_width(card, 360);
    lv_obj_center(card);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(card, 18, 0);
    lv_obj_set_style_pad_row(card, 10, 0);

    lv_obj_t *badge = ui_icon_badge(card, UI_SYM_FILE_EXPORT, UI_ICON_LG, UI_C_TEAL, UI_C_CARD_BG2, 44);
    lv_obj_set_style_radius(badge, 12, 0);

    ui_label(card, _t("Mantenimiento · microSD"), UI_FONT_TITLE, UI_C_TEXT);
    ui_label(card, _t("AppConfig · certificados AWS"), UI_FONT_XS, UI_C_TEXT_MUTED);

    s_bar = lv_bar_create(card);
    lv_obj_set_size(s_bar, LV_PCT(100), 8);
    lv_bar_set_range(s_bar, 0, 100);
    lv_bar_set_value(s_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar, ui_col(0x22262d), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar, ui_col(UI_C_TEAL), LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_bar, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(s_bar, 4, LV_PART_INDICATOR);

    s_status = ui_label(card, _t("Tarjeta detectada…"), UI_FONT_SM, UI_C_TEXT_2);

    /* botón _t("Aplicar y reiniciar") (oculto hasta terminar con cambios) */
    s_btn = ui_box(card);
    lv_obj_set_size(s_btn, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(s_btn, ui_col(UI_C_TEAL), 0);
    lv_obj_set_style_bg_opa(s_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_btn, UI_RADIUS_SM, 0);
    lv_obj_set_style_pad_ver(s_btn, 10, 0);
    lv_obj_set_flex_flow(s_btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(s_btn, 6, 0);
    lv_obj_add_flag(s_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_btn, LV_OBJ_FLAG_HIDDEN);
    ui_icon(s_btn, UI_SYM_REFRESH, UI_ICON_SM, 0x06251f);
    ui_label(s_btn, _t("Aplicar y reiniciar"), UI_FONT_MD, 0x06251f);
}

void ui_sd_progress(int pct, const char *status)
{
    if (!s_overlay) return;
    if (s_bar) lv_bar_set_value(s_bar, pct, LV_ANIM_ON);
    if (s_status && status) lv_label_set_text(s_status, _t(status));
}

void ui_sd_finish_restart(lv_event_cb_t restart_cb)
{
    if (!s_overlay || !s_btn) return;
    if (s_status) lv_label_set_text(s_status, _t("Cambios aplicados. Se requiere reinicio."));
    lv_obj_remove_flag(s_btn, LV_OBJ_FLAG_HIDDEN);
    if (restart_cb) lv_obj_add_event_cb(s_btn, restart_cb, LV_EVENT_CLICKED, NULL);
}

void ui_sd_close(void)
{
    if (!s_overlay) return;
    lv_obj_del(s_overlay);
    s_overlay = NULL; s_bar = NULL; s_status = NULL; s_btn = NULL;
}

#include "ui_generalSimpleScreen.h"
#include "ui_i18n.h"
#include "ui_widgets.h"
#include "ui_theme.h"
#include "ui_cfg.h"
#include "ui_nav.h"
#include <string.h>

/* Ajustes generales (modo simple / local, mockup 5e). */

lv_obj_t *ui_generalSimpleScreen = NULL;

static lv_obj_t *s_br_pct = NULL;   /* label "NN%" junto al slider de brillo */
static lv_obj_t *s_tz_val = NULL;   /* label de la zona horaria actual */

/* Idioma: alterna es⇄en, guarda, y reconstruye ESTA pantalla en su lugar
 * (el resto de pantallas se recrean al abrirse; el main se retraduce vía
 * ui_main_apply_config dentro de ui_cfg_set_lang). */
static void lang_row_cb(lv_event_t *e)
{
    (void)e;
    ui_cfg_set_lang(strcmp(ui_cfg_lang(), "en") == 0 ? "es" : "en");
    lv_obj_t *old = ui_generalSimpleScreen;
    ui_generalSimpleScreen = NULL;
    s_br_pct = NULL; s_tz_val = NULL;
    ui_generalSimpleScreen_screen_init();
    ui_nav_replace(ui_generalSimpleScreen);          /* sustituye el tope de la pila */
    lv_obj_delete_delayed(old, 400);                 /* libera tras la animación */
}

/* Zona horaria: cicla la lista corta y aplica TZ al RTC. */
static void tz_row_cb(lv_event_t *e)
{
    (void)e;
    int nxt = (ui_cfg_tz_index() + 1) % ui_cfg_tz_count();
    ui_cfg_set_tz_index(nxt);
    if (s_tz_val) lv_label_set_text(s_tz_val, ui_cfg_tz_label(nxt));
}

/* Arrastrar = preview en el backlight; soltar = persistir en NVS. */
static void brightness_slider_cb(lv_event_t *e)
{
    lv_obj_t *sl = lv_event_get_target(e);
    int v = (int)lv_slider_get_value(sl);
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        if (s_br_pct) lv_label_set_text_fmt(s_br_pct, "%d%%", v);
        ui_cfg_preview_brightness(v);
    } else { /* LV_EVENT_RELEASED */
        ui_cfg_set_brightness(v);
    }
}

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

    lv_obj_t *hdr = ui_nav_header(ui_generalSimpleScreen, _t("Ajustes generales"));
    ui_label(hdr, _t("local · sin red"), UI_FONT_XS, UI_C_TEXT_3);

    lv_obj_t *rows = ui_box(ui_generalSimpleScreen);
    lv_obj_set_width(rows, LV_PCT(100));
    lv_obj_set_flex_grow(rows, 1);
    lv_obj_set_flex_flow(rows, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(rows, 8, 0);

    /* Idioma (funcional: toca para alternar es⇄en) */
    lv_obj_t *lang = simple_row(rows, UI_SYM_LANGUAGE, UI_C_OK, _t("Idioma"), _t("idioma de la interfaz"));
    ui_label(lang, strcmp(ui_cfg_lang(), "en") == 0 ? "English" : "Español", UI_FONT_SM, 0xe6e8eb);
    ui_icon(lang, UI_SYM_CHEVRON_RIGHT, UI_ICON_SM, UI_C_TEXT_MUTED);
    lv_obj_add_flag(lang, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lang, lang_row_cb, LV_EVENT_CLICKED, NULL);

    /* Zona horaria (funcional: toca para ciclar; aplica TZ del RTC) */
    lv_obj_t *tz = simple_row(rows, UI_SYM_CLOCK, UI_C_WARN_SOFT, _t("Zona horaria"),
                              _t("RTC RV-3028 · hora local de eventos"));
    s_tz_val = ui_label(tz, ui_cfg_tz_label(ui_cfg_tz_index()), UI_FONT_SM, 0xe6e8eb);
    ui_icon(tz, UI_SYM_CHEVRON_RIGHT, UI_ICON_SM, UI_C_TEXT_MUTED);
    lv_obj_add_flag(tz, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tz, tz_row_cb, LV_EVENT_CLICKED, NULL);

    /* Brillo con slider (funcional: preview al arrastrar, guarda al soltar) */
    int br_now = ui_cfg_brightness();
    lv_obj_t *br = simple_row(rows, UI_SYM_BRIGHTNESS_UP, UI_C_BLUE, _t("Brillo"),
                             _t("reposo al 50% tras inactividad"));
    lv_obj_t *sl = lv_slider_create(br);
    lv_obj_set_size(sl, 96, 6);
    lv_slider_set_range(sl, 10, 100);
    lv_slider_set_value(sl, br_now, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(sl, ui_col(0x22262d), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sl, ui_col(UI_C_BLUE), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sl, ui_col(UI_C_BLUE), LV_PART_KNOB);
    lv_obj_set_style_pad_all(sl, 4, LV_PART_KNOB);          /* knob compacto */
    lv_obj_set_ext_click_area(sl, 12);                      /* área táctil cómoda */
    lv_obj_add_event_cb(sl, brightness_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(sl, brightness_slider_cb, LV_EVENT_RELEASED, NULL);
    s_br_pct = ui_label(br, "", UI_FONT_SM, UI_C_TEXT_2);
    lv_label_set_text_fmt(s_br_pct, "%d%%", br_now);
    lv_obj_set_width(s_br_pct, 42);                          /* ancho fijo: no re-fluye al cambiar */
    lv_obj_set_style_text_align(s_br_pct, LV_TEXT_ALIGN_RIGHT, 0);

    /* footer */
    lv_obj_t *foot = ui_box(ui_generalSimpleScreen);
    lv_obj_set_size(foot, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(foot, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(foot, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(foot, 6, 0);
    ui_icon(foot, UI_SYM_DEVICE_MOBILE_MESSAGE, UI_ICON_SM, UI_C_TEXT_MUTED);
    ui_label(foot, _t("Red, nube y nombre del equipo se gestionan desde la app por BLE"),
             UI_FONT_XS, UI_C_TEXT_MUTED);
}

void ui_generalSimpleScreen_screen_destroy(void)
{
    if (ui_generalSimpleScreen) { lv_obj_del(ui_generalSimpleScreen); ui_generalSimpleScreen = NULL;
                                  s_br_pct = NULL; s_tz_val = NULL; }
}

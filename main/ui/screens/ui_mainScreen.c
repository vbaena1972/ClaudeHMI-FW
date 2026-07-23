#include "ui_mainScreen.h"
#include "ui_i18n.h"
#include "ui_widgets.h"
#include "ui_theme.h"
#include "ui.h"
#include "alarm_mgr.h"
#include <stdio.h>
#include <string.h>

/* ============================================================
 *  Pantalla principal (mockups 3a/3b/4b/4c)
 *  Un único screen con 4 estados visuales:
 *   NORMAL / ADVERTENCIA / ALARMA / ALARMA-SILENCIADA
 * ============================================================ */

/* --- Constantes de escala ---
 * El fondo de escala del eje de flujo ahora viene de AppConfig
 * (sensors.flow_fullscale_lpm, calibrable sin recompilar); este define
 * queda solo como respaldo si la config trae un valor inválido. */
#define FLOW_AXIS_FULLSCALE_LPM   100.0f  /* respaldo (L/min) */
#define FLOW_HIGH_ZONE_FRAC       0.80f   /* inicio de zona "alta" (ámbar) en la barra */
#define PRESS_AXIS_FULLSCALE_KPA  1378.951f /* fixed 0..200 psi pressure sensor */

/* --- Globals del contrato (ui_bind / main.c) --- */
lv_obj_t *ui_mainScreen        = NULL;
lv_obj_t *ui_statusMainComm    = NULL;
lv_obj_t *ui_bluetoothStatusMain = NULL;
lv_obj_t *ui_wifiStatusMain    = NULL;
lv_obj_t *ui_ethernetStatusMain = NULL;
lv_obj_t *ui_cloudStatusMain   = NULL;
lv_obj_t *ui_alarmBtnMain      = NULL;

/* --- Tarjeta de métrica (presión/flujo) --- */
typedef struct {
    lv_obj_t *card;
    lv_obj_t *title;
    lv_obj_t *icon;
    lv_obj_t *value;
    lv_obj_t *unit;
    lv_obj_t *bar;
    lv_obj_t *seg_lo, *seg_mid, *seg_hi;
    lv_obj_t *tick_lo, *tick_hi;
    lv_obj_t *marker;
    lv_obj_t *ax_left, *ax_mid, *ax_right;
    lv_obj_t *pill;
    lv_obj_t *pill_state;
    lv_obj_t *pill_mm;
} metric_card_t;

static metric_card_t s_press, s_flow;

/* --- Consumo del día: integral del flujo (∫ L/min · dt → m³) ---
 * Se integra entre timestamps de muestra (ts_ms): si ui_refresh repite la misma
 * muestra, dt=0 y no hay doble conteo. Vive en RAM (se pierde al reiniciar). */
static float   s_consumo_m3 = 0.f;
static int64_t s_consumo_prev_ts = -1;
static char    s_clock_prev[8] = "";

/* --- Cabecera / banner / consumo / gas --- */
static lv_obj_t *s_brand_lbl, *s_sub_lbl, *s_clock_lbl, *s_date_lbl;
static lv_obj_t *s_banner, *s_banner_icon, *s_banner_txt, *s_banner_sub, *s_banner_right, *s_banner_right_ic, *s_banner_right_tx;
static lv_obj_t *s_consumo_val, *s_consumo_badge, *s_consumo_badge_tx;
static lv_obj_t *s_consumo_cap = NULL;   /* caption "CONSUMO" (retraducible) */
static lv_obj_t *s_gas_banner, *s_gas_lbl;
static lv_obj_t *s_menu_pop = NULL;
static lv_obj_t *s_alarm_overlay = NULL;
static sensor_sample_t s_alarm_last;
static bool s_alarm_have_last = false;
static alarm_clinical_state_t s_alarm_state = ALARM_STATE_NORMAL;
static bool s_alarm_muted = false;
static uint32_t s_alarm_faults = 0;


/* ---------- helpers ---------- */
static void set_bg(lv_obj_t *o, uint32_t hex)
{ lv_obj_set_style_bg_color(o, ui_col(hex), LV_PART_MAIN | LV_STATE_DEFAULT); }
static void set_border(lv_obj_t *o, uint32_t hex)
{ lv_obj_set_style_border_color(o, ui_col(hex), LV_PART_MAIN | LV_STATE_DEFAULT); }
static void set_txt_color(lv_obj_t *o, uint32_t hex)
{ lv_obj_set_style_text_color(o, ui_col(hex), LV_PART_MAIN | LV_STATE_DEFAULT); }

static float clampf(float v, float lo, float hi){ return v < lo ? lo : (v > hi ? hi : v); }

/* Posiciona un overlay (marker/tick) dentro de la barra según una fracción 0..1 */
static void position_overlay(lv_obj_t *obj, lv_obj_t *bar, float frac, lv_coord_t obj_w)
{
    lv_coord_t w = lv_obj_get_content_width(bar);
    if (w <= 0) w = 150; /* fallback antes del primer layout */
    lv_coord_t x = (lv_coord_t)(clampf(frac, 0.f, 1.f) * (float)(w - obj_w));
    lv_obj_align(obj, LV_ALIGN_LEFT_MID, x, 0);
}

/* ---------- builder de la tarjeta de métrica ---------- */
static void build_metric_card(lv_obj_t *parent, metric_card_t *m,
                              const char *title, const char *icon_sym, const char *unit)
{
    m->card = ui_card(parent);
    lv_obj_set_flex_grow(m->card, 1);
    lv_obj_set_height(m->card, LV_PCT(100));
    lv_obj_set_flex_flow(m->card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(m->card, 6, 0);
    lv_obj_set_style_pad_row(m->card, 2, 0);

    /* fila título + icono */
    lv_obj_t *row = ui_box(m->card);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    m->title = ui_label(row, title, UI_FONT_XS, UI_C_TEXT_3);
    lv_obj_set_style_text_letter_space(m->title, 1, 0);
    m->icon = ui_icon(row, icon_sym, UI_ICON_MD, UI_C_TEXT_MUTED);

    /* valor grande + unidad */
    lv_obj_t *vrow = ui_box(m->card);
    lv_obj_set_size(vrow, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(vrow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(vrow, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(vrow, 5, 0);
    m->value = ui_label(vrow, "--", UI_FONT_HUGE, UI_C_TEXT_STRONG);
    m->unit  = ui_label(vrow, unit, UI_FONT_LG, UI_C_TEXT_2);

    /* barra de rango (flex de 3 segmentos) */
    m->bar = ui_box(m->card);
    lv_obj_set_size(m->bar, LV_PCT(100), 13);
    lv_obj_set_style_radius(m->bar, 7, 0);
    lv_obj_set_style_clip_corner(m->bar, true, 0);
    set_bg(m->bar, UI_C_BORDER_SOFT);
    lv_obj_set_style_bg_opa(m->bar, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(m->bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(m->bar, 0, 0);

    m->seg_lo  = ui_box(m->bar); lv_obj_set_height(m->seg_lo, LV_PCT(100));
    m->seg_mid = ui_box(m->bar); lv_obj_set_height(m->seg_mid, LV_PCT(100));
    m->seg_hi  = ui_box(m->bar); lv_obj_set_height(m->seg_hi, LV_PCT(100));
    lv_obj_set_style_bg_opa(m->seg_lo, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_opa(m->seg_mid, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_opa(m->seg_hi, LV_OPA_COVER, 0);

    /* ticks de umbral (overlay) */
    m->tick_lo = ui_box(m->bar);
    m->tick_hi = ui_box(m->bar);
    for (int i = 0; i < 2; i++) {
        lv_obj_t *t = i ? m->tick_hi : m->tick_lo;
        lv_obj_add_flag(t, LV_OBJ_FLAG_IGNORE_LAYOUT);
        lv_obj_set_size(t, 2, 13);
        set_bg(t, UI_C_OK_SOFT);
        lv_obj_set_style_bg_opa(t, LV_OPA_COVER, 0);
    }

    /* marcador (overlay) — cabe dentro de la barra (LVGL recorta hijos al padre) */
    m->marker = ui_box(m->bar);
    lv_obj_add_flag(m->marker, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(m->marker, 9, 13);
    lv_obj_set_style_radius(m->marker, 3, 0);
    set_bg(m->marker, UI_C_OK);
    lv_obj_set_style_bg_opa(m->marker, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(m->marker, ui_col(UI_C_CARD_BG), 0);
    lv_obj_set_style_border_width(m->marker, 2, 0);

    /* eje: 0 · centro · max */
    lv_obj_t *axrow = ui_box(m->card);
    lv_obj_set_size(axrow, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(axrow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(axrow, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    m->ax_left  = ui_label(axrow, "0", UI_FONT_XS, UI_C_TEXT_MUTED);
    m->ax_mid   = ui_label(axrow, "", UI_FONT_XS, UI_C_OK_DIM);
    m->ax_right = ui_label(axrow, "", UI_FONT_XS, UI_C_TEXT_MUTED);

    /* espaciador flexible: empuja la píldora al fondo */
    lv_obj_t *spacer = ui_box(m->card);
    lv_obj_set_width(spacer, LV_PCT(100));
    lv_obj_set_flex_grow(spacer, 1);

    /* pill de estado (empuja al fondo) */
    m->pill = ui_box(m->card);
    lv_obj_set_width(m->pill, LV_PCT(100));
    lv_obj_set_height(m->pill, LV_SIZE_CONTENT);
    lv_obj_set_style_margin_top(m->pill, 2, 0);
    lv_obj_set_flex_grow(m->pill, 0);
    lv_obj_set_style_radius(m->pill, UI_RADIUS_PILL, 0);
    lv_obj_set_style_border_width(m->pill, 1, 0);
    lv_obj_set_style_pad_hor(m->pill, 10, 0);
    lv_obj_set_style_pad_ver(m->pill, 3, 0);
    lv_obj_set_flex_flow(m->pill, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(m->pill, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    m->pill_state = ui_label(m->pill, _t("NORMAL"), UI_FONT_SM, UI_C_OK);
    lv_obj_set_style_text_letter_space(m->pill_state, 1, 0);
    m->pill_mm = ui_label(m->pill, "24H -- / --", UI_FONT_XS, UI_C_TEXT_MUTED);
}

/* Actualiza una tarjeta con valor + zonas + estado */
typedef enum { CARD_OK = 0, CARD_WARN, CARD_ALARM } card_state_t;

static void update_metric_card(metric_card_t *m, float value_disp, float frac,
                               float safe_lo_frac, float safe_hi_frac, bool low_zone,
                               card_state_t st, const char *state_txt,
                               float mn_disp, float mx_disp)
{
    char buf[48];
    snprintf(buf, sizeof(buf), "%.0f", value_disp);
    lv_label_set_text(m->value, buf);

    /* colores por estado */
    uint32_t accent = (st == CARD_ALARM) ? UI_C_ALARM_SOFT : (st == CARD_WARN ? UI_C_WARN : UI_C_OK);
    uint32_t valcol = (st == CARD_OK) ? UI_C_TEXT_STRONG : accent;
    set_txt_color(m->value, valcol);
    set_txt_color(m->title, (st == CARD_OK) ? UI_C_TEXT_3 : accent);
    set_txt_color(m->icon,  (st == CARD_OK) ? UI_C_TEXT_MUTED : accent);
    set_border(m->card, (st == CARD_OK) ? UI_C_BORDER : accent);
    lv_obj_set_style_border_opa(m->card, (st == CARD_OK) ? LV_OPA_60 : LV_OPA_COVER, 0);

    /* zonas: pct de cada segmento */
    int lo = (int)(clampf(low_zone ? safe_lo_frac : 0.f, 0.f, 1.f) * 100.f);
    int hi = (int)(clampf(1.f - safe_hi_frac, 0.f, 1.f) * 100.f);
    int mid = 100 - lo - hi; if (mid < 0) mid = 0;
    lv_obj_set_width(m->seg_lo, LV_PCT(lo));
    lv_obj_set_width(m->seg_mid, LV_PCT(mid));
    lv_obj_set_width(m->seg_hi, LV_PCT(hi));
    set_bg(m->seg_lo, UI_C_ALARM);   lv_obj_set_style_bg_opa(m->seg_lo, LV_OPA_20, 0);
    set_bg(m->seg_mid, UI_C_OK);     lv_obj_set_style_bg_opa(m->seg_mid, LV_OPA_30, 0);
    set_bg(m->seg_hi, low_zone ? UI_C_ALARM : UI_C_WARN);
    lv_obj_set_style_bg_opa(m->seg_hi, LV_OPA_20, 0);

    /* marcador color + posición */
    set_bg(m->marker, accent);
    position_overlay(m->marker, m->bar, frac, 9);
    if (low_zone) {
        lv_obj_clear_flag(m->tick_lo, LV_OBJ_FLAG_HIDDEN);
        position_overlay(m->tick_lo, m->bar, safe_lo_frac, 2);
    } else {
        lv_obj_add_flag(m->tick_lo, LV_OBJ_FLAG_HIDDEN);
    }
    position_overlay(m->tick_hi, m->bar, safe_hi_frac, 2);
    set_bg(m->tick_hi, low_zone ? UI_C_OK_SOFT : UI_C_WARN_SOFT);

    /* pill de estado */
    uint32_t pill_bg = (st == CARD_ALARM) ? UI_C_ALARM_BG : (st == CARD_WARN ? UI_C_WARN_BG : UI_C_OK_BG);
    uint32_t pill_bd = (st == CARD_ALARM) ? UI_C_ALARM_BORDER : (st == CARD_WARN ? UI_C_WARN_BORDER : UI_C_OK_BORDER);
    set_bg(m->pill, pill_bg);
    set_border(m->pill, pill_bd);
    lv_obj_set_style_bg_opa(m->pill, LV_OPA_40, 0);
    lv_label_set_text(m->pill_state, state_txt);
    set_txt_color(m->pill_state, accent);
    snprintf(buf, sizeof(buf), "24H %.0f / %.0f", mn_disp, mx_disp);
    lv_label_set_text(m->pill_mm, buf);
}

/* ---------- conversiones de unidades ----------
 * bar era kPa/10 en el FW original: bug (1 bar = 100 kPa), corregido a /100.
 * Mantiene la firma (valor, unit) para usar el cfg que recibe ui_main_update. */
static float pressure_to_disp(float kpa, const char *unit)
{
    if (unit && strcmp(unit, "psi") == 0) return kpa * 0.145038f;
    if (unit && strcmp(unit, "bar") == 0) return kpa / 100.0f;
    if (unit && strcmp(unit, "mpa") == 0) return kpa / 1000.0f;
    return kpa; /* kpa */
}
static float flow_to_disp(float lpm, const char *unit)
{
    if (unit && strcmp(unit, "sccm") == 0) return lpm * 1000.0f;
    if (unit && strcmp(unit, "m3h") == 0)  return lpm * 0.06f;  /* 1 L/min = 0.06 m³/h */
    return lpm; /* lpm/slpm/nlpm */
}

/* ---------- callback de mute (tap en el banner) ---------- */
static void alarm_overlay_open(void);
static void banner_open_cb(lv_event_t *e) { (void)e; alarm_overlay_open(); }
static void menu_close(void) { if (s_menu_pop) { lv_obj_delete_async(s_menu_pop); s_menu_pop = NULL; } }
static void menu_scrim_cb(lv_event_t *e) { (void)e; menu_close(); }
static void menu_item_cb(lv_event_t *e) { int w=(int)(intptr_t)lv_event_get_user_data(e); menu_close(); if(w==0) ui_open_info_cb(e); else if(w==2) ui_open_config_pin_cb(e); }
static void menu_add_item(lv_obj_t *p,const char *sym,const char *txt,int w,bool en)
{
    lv_obj_t *r=ui_box(p); lv_obj_set_size(r,LV_PCT(100),38); lv_obj_set_flex_flow(r,LV_FLEX_FLOW_ROW); lv_obj_set_flex_align(r,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER); lv_obj_set_style_pad_hor(r,9,0); lv_obj_set_style_pad_column(r,9,0); lv_obj_set_style_radius(r,7,0); ui_label(r,sym,UI_FONT_SM,en?UI_C_TEXT_3:UI_C_TEXT_MUTED); ui_label(r,txt,UI_FONT_SM,en?UI_C_TEXT:UI_C_TEXT_MUTED); if(en){lv_obj_add_flag(r,LV_OBJ_FLAG_CLICKABLE);lv_obj_add_event_cb(r,menu_item_cb,LV_EVENT_CLICKED,(void*)(intptr_t)w);}
}
static void menu_open_cb(lv_event_t *e)
{
    (void)e; if(s_menu_pop){menu_close();return;} s_menu_pop=lv_obj_create(lv_layer_top()); lv_obj_remove_style_all(s_menu_pop); lv_obj_set_size(s_menu_pop,LV_PCT(100),LV_PCT(100)); lv_obj_add_flag(s_menu_pop,LV_OBJ_FLAG_CLICKABLE); lv_obj_clear_flag(s_menu_pop,LV_OBJ_FLAG_SCROLLABLE); lv_obj_add_event_cb(s_menu_pop,menu_scrim_cb,LV_EVENT_CLICKED,NULL); lv_obj_t *p=ui_card(s_menu_pop); lv_obj_set_size(p,190,LV_SIZE_CONTENT); lv_obj_set_pos(p,278,46); lv_obj_set_style_pad_all(p,6,0); lv_obj_set_style_pad_row(p,2,0); lv_obj_set_style_shadow_width(p,24,0); lv_obj_set_style_shadow_opa(p,LV_OPA_40,0); lv_obj_set_flex_flow(p,LV_FLEX_FLOW_COLUMN); menu_add_item(p,LV_SYMBOL_EYE_OPEN,"Información",0,true); menu_add_item(p,LV_SYMBOL_FILE,"Logs",1,true); menu_add_item(p,LV_SYMBOL_SETTINGS,"Configuración",2,true);
}
static const char *alarm_condition(const AppConfig *c){if(s_alarm_faults)return "FALLA MODULO SENSORES";if(!s_alarm_have_last||!c)return "FALLA DE MEDICION";if(s_alarm_last.pressure_kpa<c->sensors.alarm_limits.pressure_min)return "PRESION BAJA";if(s_alarm_last.pressure_kpa>c->sensors.alarm_limits.pressure_max)return "PRESION ALTA";return "ALARMA DE FLUJO";}
static void alarm_overlay_close(void){if(s_alarm_overlay){lv_obj_delete_async(s_alarm_overlay);s_alarm_overlay=NULL;}}
static void alarm_overlay_close_cb(lv_event_t *e){(void)e;alarm_overlay_close();}
static void alarm_overlay_silence_cb(lv_event_t *e){(void)e;if(!s_alarm_muted)alarm_mgr_press_mute();alarm_overlay_close();}
static void alarm_overlay_open(void)
{
    if (s_alarm_overlay || s_alarm_state == ALARM_STATE_NORMAL) return;
    const AppConfig *c=appcfg_cache_peek(); bool crit=s_alarm_state==ALARM_STATE_ALERT; uint32_t ac=crit?UI_C_ALARM:UI_C_WARN;
    s_alarm_overlay=lv_obj_create(lv_layer_top()); lv_obj_set_size(s_alarm_overlay,LV_PCT(100),LV_PCT(100)); lv_obj_set_style_bg_color(s_alarm_overlay,ui_col(crit?0x210d10:0x211a08),0); lv_obj_set_style_bg_opa(s_alarm_overlay,LV_OPA_COVER,0);
    lv_obj_set_style_border_width(s_alarm_overlay,5,0); lv_obj_set_style_border_color(s_alarm_overlay,ui_col(ac),0); lv_obj_set_style_pad_all(s_alarm_overlay,14,0); lv_obj_set_flex_flow(s_alarm_overlay,LV_FLEX_FLOW_COLUMN); lv_obj_clear_flag(s_alarm_overlay,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *h=ui_box(s_alarm_overlay);lv_obj_set_size(h,LV_PCT(100),36);lv_obj_set_flex_flow(h,LV_FLEX_FLOW_ROW);lv_obj_set_flex_align(h,LV_FLEX_ALIGN_SPACE_BETWEEN,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);lv_obj_t *hl=ui_box(h);lv_obj_set_size(hl,LV_SIZE_CONTENT,36);lv_obj_set_flex_flow(hl,LV_FLEX_FLOW_ROW);lv_obj_set_flex_align(hl,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);lv_obj_set_style_pad_column(hl,8,0);ui_icon(hl,UI_SYM_ALERT_TRIANGLE,UI_ICON_MD,ac);ui_label(hl,crit?"ALARMA":"ADVERTENCIA",UI_FONT_XL,0xffe3e3);lv_obj_t *bk=ui_icon_badge(h,UI_SYM_ARROW_LEFT,UI_ICON_SM,UI_C_TEXT,0x35191c,34);lv_obj_add_flag(bk,LV_OBJ_FLAG_CLICKABLE);lv_obj_add_event_cb(bk,alarm_overlay_close_cb,LV_EVENT_CLICKED,NULL);
    lv_obj_t *ct=ui_box(s_alarm_overlay);lv_obj_set_width(ct,LV_PCT(100));lv_obj_set_flex_grow(ct,1);lv_obj_set_flex_flow(ct,LV_FLEX_FLOW_COLUMN);lv_obj_set_flex_align(ct,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);lv_obj_set_style_pad_row(ct,5,0);ui_label(ct,alarm_condition(c),UI_FONT_XL,crit?UI_C_ALARM_SOFT:UI_C_WARN_SOFT);char val[24]="--",unit[12]="",lim[64]="";
    if(s_alarm_faults){snprintf(lim,sizeof(lim),"sin datos validos - revisar modulo y cable I2C");}else if(s_alarm_have_last&&c){bool p=s_alarm_last.pressure_kpa<c->sensors.alarm_limits.pressure_min||s_alarm_last.pressure_kpa>c->sensors.alarm_limits.pressure_max;if(p){float v=pressure_to_disp(s_alarm_last.pressure_kpa,c->sensors.pressure_unit);float t=pressure_to_disp(s_alarm_last.pressure_kpa<c->sensors.alarm_limits.pressure_min?c->sensors.alarm_limits.pressure_min:c->sensors.alarm_limits.pressure_max,c->sensors.pressure_unit);snprintf(val,sizeof(val),"%.0f",(double)v);snprintf(unit,sizeof(unit),"%s",c->sensors.pressure_unit);snprintf(lim,sizeof(lim),"valor %s %.0f %s",s_alarm_last.pressure_kpa<c->sensors.alarm_limits.pressure_min?"<":">",(double)t,c->sensors.pressure_unit);}else{snprintf(val,sizeof(val),"%.0f",(double)flow_to_disp(s_alarm_last.flow_lpm,c->sensors.flow_unit));snprintf(unit,sizeof(unit),"%s",c->sensors.flow_unit);snprintf(lim,sizeof(lim),"flujo por encima de lo habitual");}}
    lv_obj_t *vr=ui_box(ct);lv_obj_set_size(vr,LV_SIZE_CONTENT,LV_SIZE_CONTENT);lv_obj_set_flex_flow(vr,LV_FLEX_FLOW_ROW);lv_obj_set_flex_align(vr,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_END,LV_FLEX_ALIGN_END);lv_obj_set_style_pad_column(vr,7,0);ui_label(vr,val,UI_FONT_HUGE,UI_C_TEXT_STRONG);ui_label(vr,unit,UI_FONT_LG,UI_C_TEXT_2);ui_label(ct,lim,UI_FONT_SM,crit?UI_C_ALARM_SOFT:UI_C_WARN_DIM);
    lv_obj_t *s=ui_box(s_alarm_overlay);lv_obj_set_size(s,LV_PCT(100),48);ui_style_button(s,0xf1f3f5);lv_obj_add_flag(s,LV_OBJ_FLAG_CLICKABLE);lv_obj_add_event_cb(s,crit?alarm_overlay_silence_cb:alarm_overlay_close_cb,LV_EVENT_CLICKED,NULL);lv_obj_center(ui_label(s,(!crit||s_alarm_muted)?"Volver":"Silenciar alarma",UI_FONT_MD,0x2a1416));
}

/* ---------- construcción de la pantalla ---------- */
void ui_mainScreen_screen_init(void)
{
    ui_mainScreen = ui_screen_base();
    lv_obj_set_flex_flow(ui_mainScreen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_ver(ui_mainScreen, 5, 0);
    lv_obj_set_style_pad_hor(ui_mainScreen, 12, 0);
    lv_obj_set_style_pad_row(ui_mainScreen, 4, 0);

    /* ===== Header ===== */
    lv_obj_t *hdr = ui_box(ui_mainScreen);
    lv_obj_set_size(hdr, LV_PCT(100), 36);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *brand = ui_box(hdr);
    lv_obj_set_size(brand, LV_SIZE_CONTENT, 36);
    lv_obj_set_flex_flow(brand, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(brand, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(brand, 9, 0);
    ui_icon_badge(brand, UI_SYM_ACTIVITY, UI_ICON_MD, 0x9fe1cb, UI_C_HEADER_ICONBG, 30);
    lv_obj_t *bcol = ui_box(brand);
    lv_obj_set_size(bcol, 116, 32);
    s_brand_lbl = ui_label(bcol, "Ubicacion", UI_FONT_SM, UI_C_TEXT);
    lv_obj_set_size(s_brand_lbl, 116, 16); lv_obj_set_pos(s_brand_lbl, 0, 0);
    s_sub_lbl   = ui_label(bcol, "Axira - FPM-200", UI_FONT_XS, UI_C_TEXT_3);
    lv_obj_set_size(s_sub_lbl, 116, 16); lv_obj_set_pos(s_sub_lbl, 0, 16);

    /* fila de estado (iconos + reloj + settings) */
    ui_statusMainComm = ui_box(hdr);
    lv_obj_set_size(ui_statusMainComm, LV_SIZE_CONTENT, 36);
    lv_obj_set_flex_flow(ui_statusMainComm, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_statusMainComm, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(ui_statusMainComm, 7, 0);
    ui_bluetoothStatusMain = ui_icon(ui_statusMainComm, UI_SYM_BLUETOOTH, UI_ICON_SM, UI_C_BLUE);
    ui_wifiStatusMain      = ui_icon(ui_statusMainComm, UI_SYM_WIFI, UI_ICON_SM, UI_C_OK);
    ui_ethernetStatusMain  = ui_icon(ui_statusMainComm, UI_SYM_NETWORK, UI_ICON_SM, UI_C_TEXT_2);
    ui_cloudStatusMain     = ui_icon(ui_statusMainComm, UI_SYM_CLOUD_CHECK, UI_ICON_SM, UI_C_TEAL);
    lv_obj_t *sep = ui_box(ui_statusMainComm); lv_obj_set_size(sep, 1, 24); lv_obj_set_style_bg_color(sep, ui_col(UI_C_BORDER), 0); lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_t *dt = ui_box(ui_statusMainComm); lv_obj_set_size(dt,72,32);
    s_clock_lbl = ui_label(dt, "--:--", UI_FONT_XS, UI_C_TEXT); lv_obj_set_size(s_clock_lbl,72,16); lv_obj_set_pos(s_clock_lbl,0,0); lv_obj_set_style_text_align(s_clock_lbl,LV_TEXT_ALIGN_RIGHT,0);
    s_date_lbl  = ui_label(dt, "--/--/----", UI_FONT_XS, UI_C_TEXT_MUTED); lv_obj_set_size(s_date_lbl,72,16); lv_obj_set_pos(s_date_lbl,0,16); lv_obj_set_style_text_align(s_date_lbl,LV_TEXT_ALIGN_RIGHT,0);
    lv_obj_t *menu = ui_box(ui_statusMainComm); lv_obj_set_size(menu,30,30); lv_obj_set_style_bg_color(menu,lv_color_hex(UI_C_CARD_BG),0); lv_obj_set_style_bg_opa(menu,LV_OPA_COVER,0); lv_obj_set_style_border_width(menu,1,0); lv_obj_set_style_border_color(menu,ui_col(UI_C_BORDER),0); lv_obj_set_style_radius(menu,7,0); lv_obj_set_flex_flow(menu,LV_FLEX_FLOW_ROW); lv_obj_set_flex_align(menu,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER); ui_label(menu,LV_SYMBOL_LIST,UI_FONT_SM,UI_C_TEXT);
    lv_obj_add_flag(menu, LV_OBJ_FLAG_CLICKABLE); lv_obj_set_ext_click_area(menu, 7); lv_obj_add_event_cb(menu, menu_open_cb, LV_EVENT_CLICKED, NULL);

    /* ===== Banner de estado ===== */
    ui_alarmBtnMain = ui_box(ui_mainScreen);
    s_banner = ui_alarmBtnMain;
    lv_obj_set_size(s_banner, LV_PCT(100), 30);
    lv_obj_set_style_radius(s_banner, UI_RADIUS_SM, 0);
    lv_obj_set_style_border_width(s_banner, 1, 0);
    lv_obj_set_style_pad_hor(s_banner, 12, 0);
    lv_obj_set_style_pad_ver(s_banner, 0, 0);
    lv_obj_set_flex_flow(s_banner, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_banner, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(s_banner, 8, 0);
    lv_obj_add_flag(s_banner, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_banner, banner_open_cb, LV_EVENT_CLICKED, NULL);
    s_banner_icon = ui_icon(s_banner, UI_SYM_CIRCLE_CHECK, UI_ICON_SM, UI_C_OK);
    s_banner_txt  = ui_label(s_banner, _t("SISTEMA NORMAL"), UI_FONT_SM, UI_C_OK_SOFT);
    lv_obj_set_style_text_letter_space(s_banner_txt, 1, 0);
    s_banner_sub  = ui_label(s_banner, _t("· sin alarmas"), UI_FONT_XS, UI_C_OK_DIM);
    /* pill derecho (para estado silenciado) */
    s_banner_right = ui_box(s_banner);
    lv_obj_set_size(s_banner_right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_margin_left(s_banner_right, 6, 0);
    lv_obj_set_flex_flow(s_banner_right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_banner_right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(s_banner_right, 5, 0);
    lv_obj_set_flex_grow(s_banner_right, 1);
    s_banner_right_ic = ui_icon(s_banner_right, UI_SYM_BELL_OFF, UI_ICON_SM, UI_C_ALARM_SOFT);
    s_banner_right_tx = ui_label(s_banner_right, "", UI_FONT_XS, UI_C_ALARM_SOFT);
    lv_obj_add_flag(s_banner_right, LV_OBJ_FLAG_HIDDEN);

    /* ===== Tarjetas ===== */
    lv_obj_t *cards = ui_box(ui_mainScreen);
    lv_obj_set_size(cards, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(cards, 1);
    lv_obj_set_flex_flow(cards, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(cards, 10, 0);
    build_metric_card(cards, &s_press, _t("PRESIÓN"), UI_SYM_GAUGE, "psi");
    build_metric_card(cards, &s_flow,  _t("FLUJO"),   UI_SYM_WIND,  "SCCM");

    /* ===== Fila de consumo ===== */
    lv_obj_t *cons = ui_card(ui_mainScreen);
    lv_obj_set_size(cons, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_radius(cons, UI_RADIUS_TILE, 0);
    lv_obj_set_style_pad_hor(cons, 13, 0);
    lv_obj_set_style_pad_ver(cons, 3, 0);
    lv_obj_set_flex_flow(cons, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cons, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(cons, 7, 0);
    ui_icon(cons, UI_SYM_CHART_BAR, UI_ICON_SM, UI_C_TEXT_3);
    s_consumo_cap = ui_label(cons, _t("CONSUMO"), UI_FONT_XS, UI_C_TEXT_3);
    lv_obj_set_style_text_letter_space(s_consumo_cap, 1, 0);
    s_consumo_val = ui_label(cons, "-- m³ hoy", UI_FONT_XL, UI_C_TEXT_STRONG);
    lv_obj_set_style_margin_left(s_consumo_val, 4, 0);
    s_consumo_badge = ui_pill(cons, "--", UI_FONT_XS, UI_C_OK_DIM, UI_C_OK_BG, UI_C_OK_BORDER);
    lv_obj_set_style_margin_left(s_consumo_badge, 8, 0);
    s_consumo_badge_tx = lv_obj_get_child(s_consumo_badge, 0);
    lv_obj_add_flag(s_consumo_badge, LV_OBJ_FLAG_HIDDEN); /* oculto hasta tener datos reales (consumo m³) */

    /* ===== Banner de gas ===== */
    s_gas_banner = ui_box(ui_mainScreen);
    lv_obj_set_size(s_gas_banner, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_radius(s_gas_banner, UI_RADIUS_SM, 0);
    set_bg(s_gas_banner, UI_C_GAS_O2);
    lv_obj_set_style_bg_opa(s_gas_banner, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_ver(s_gas_banner, 4, 0);
    lv_obj_set_flex_flow(s_gas_banner, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_gas_banner, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    s_gas_lbl = ui_label(s_gas_banner, _t("OXÍGENO"), UI_FONT_TITLE, 0xffffff);
    lv_obj_set_style_text_letter_space(s_gas_lbl, 3, 0);

    /* estado inicial */
    ui_main_apply_config(NULL);
}

void ui_mainScreen_screen_destroy(void)
{
    if (ui_mainScreen) { lv_obj_del(ui_mainScreen); ui_mainScreen = NULL; }
}

/* ---------- API pública de binding ---------- */
void ui_main_set_clock(const char *hhmm)
{
    if (!hhmm) return;
    /* cambio de día: la hora "retrocede" (23:59 -> 00:00) => resetear consumo */
    if (s_clock_prev[0] && strcmp(hhmm, s_clock_prev) < 0)
        s_consumo_m3 = 0.f;
    strncpy(s_clock_prev, hhmm, sizeof(s_clock_prev) - 1);
    s_clock_prev[sizeof(s_clock_prev) - 1] = '\0';
    if (s_clock_lbl) lv_label_set_text(s_clock_lbl, hhmm);
}
void ui_main_set_date(const char *date)
{
    if (s_date_lbl && date) lv_label_set_text(s_date_lbl, date);
}

float ui_main_get_consumo(void) { return s_consumo_m3; }
void  ui_main_set_consumo(float m3) { if (m3 >= 0.f) s_consumo_m3 = m3; }

/* Mapea gas_type -> etiqueta + color de banner */
static void gas_label_color(const char *gas, const char **label, uint32_t *color)
{
    if (gas && strcmp(gas, "air_med") == 0) { *label = _t("AIRE MEDICINAL"); *color = UI_C_GAS_AIR; return; }
    if (gas && strcmp(gas, "n2o") == 0)     { *label = _t("ÓXIDO NITROSO");  *color = UI_C_GAS_N2O; return; }
    if (gas && strcmp(gas, "vac") == 0)     { *label = _t("VACÍO");          *color = UI_C_GAS_VAC; return; }
    *label = _t("OXÍGENO"); *color = UI_C_GAS_O2; /* o2 por defecto */
}

void ui_main_apply_config(const AppConfig *cfg)
{
    if (!cfg) cfg = appcfg_cache_peek();
    if (!cfg) return;

    if (cfg->general.client[0]) lv_label_set_text(s_brand_lbl, cfg->general.client);
    if (cfg->general.model[0])  lv_label_set_text_fmt(s_sub_lbl, "Axira - %s", cfg->general.model);

    const char *glabel; uint32_t gcolor;
    gas_label_color(cfg->sensors.gas_type, &glabel, &gcolor);
    lv_label_set_text(s_gas_lbl, glabel);
    set_bg(s_gas_banner, gcolor);

    if (cfg->sensors.pressure_unit[0]) lv_label_set_text(s_press.unit, cfg->sensors.pressure_unit);
    if (cfg->sensors.flow_unit[0])     lv_label_set_text(s_flow.unit, cfg->sensors.flow_unit);

    /* Textos estáticos retraducibles (main persiste; el resto de pantallas se
     * recrean al abrirse y no lo necesitan). save_and_refresh() pasa por aquí. */
    if (s_press.title)  lv_label_set_text(s_press.title, _t("PRESIÓN"));
    if (s_flow.title)   lv_label_set_text(s_flow.title, _t("FLUJO"));
    if (s_consumo_cap)  lv_label_set_text(s_consumo_cap, _t("CONSUMO"));
}

/* Banner por estado */
static void set_banner(alarm_clinical_state_t st, bool muted)
{
    if (st == ALARM_STATE_NORMAL) {
        set_bg(s_banner, UI_C_OK_BG); set_border(s_banner, UI_C_OK_BORDER);
        lv_obj_set_style_bg_opa(s_banner, LV_OPA_COVER, 0);
        lv_label_set_text(s_banner_icon, UI_SYM_CIRCLE_CHECK); set_txt_color(s_banner_icon, UI_C_OK);
        lv_label_set_text(s_banner_txt, _t("SISTEMA NORMAL")); set_txt_color(s_banner_txt, UI_C_OK_SOFT);
        lv_label_set_text(s_banner_sub, _t("· sin alarmas")); set_txt_color(s_banner_sub, UI_C_OK_DIM);
        lv_obj_add_flag(s_banner_right, LV_OBJ_FLAG_HIDDEN);
    } else if (st == ALARM_STATE_WARNING) {
        set_bg(s_banner, UI_C_WARN_BG); set_border(s_banner, UI_C_WARN_BORDER);
        lv_obj_set_style_bg_opa(s_banner, LV_OPA_COVER, 0);
        lv_label_set_text(s_banner_icon, UI_SYM_ALERT_TRIANGLE); set_txt_color(s_banner_icon, UI_C_WARN);
        lv_label_set_text(s_banner_txt, _t("ADVERTENCIA")); set_txt_color(s_banner_txt, UI_C_WARN_SOFT);
        lv_label_set_text(s_banner_sub, _t("· vigilar")); set_txt_color(s_banner_sub, UI_C_WARN_DIM);
        lv_obj_add_flag(s_banner_right, LV_OBJ_FLAG_HIDDEN);
    } else { /* ALERT */
        set_bg(s_banner, UI_C_ALARM_BG); set_border(s_banner, UI_C_ALARM_BORDER);
        lv_obj_set_style_bg_opa(s_banner, LV_OPA_COVER, 0);
        lv_label_set_text(s_banner_icon, UI_SYM_ALERT_TRIANGLE_FILLED); set_txt_color(s_banner_icon, UI_C_ALARM_SOFT);
        lv_label_set_text(s_banner_txt, _t("ALARMA")); set_txt_color(s_banner_txt, 0xffd3d3);
        lv_label_set_text(s_banner_sub, _t("· revisar línea")); set_txt_color(s_banner_sub, UI_C_ALARM_SOFT);
        if (muted) {
            lv_obj_clear_flag(s_banner_right, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(s_banner_right_ic, UI_SYM_BELL_OFF);
            lv_label_set_text(s_banner_right_tx, _t("silenciada"));
        } else {
            lv_obj_clear_flag(s_banner_right, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(s_banner_right_ic, UI_SYM_BELL_RINGING);
            lv_label_set_text(s_banner_right_tx, _t("buzzer activo"));
        }
    }
}

void ui_main_update(const sensor_sample_t *last, bool have_last,
                    const sensor_sample_t *mn, const sensor_sample_t *mx, bool have_mm,
                    const AppConfig *cfg,
                    alarm_clinical_state_t state, bool muted)
{
    if (!cfg) cfg = appcfg_cache_peek();
    if (!cfg) return;
    s_alarm_state = state;
    s_alarm_muted = muted;
    s_alarm_have_last = have_last;
    s_alarm_faults = alarm_mgr_get_sensor_faults();
    if (have_last) s_alarm_last = *last;

    set_banner(state, muted);

    if (have_last) {
        /* --- Presión --- */
        float p_min = cfg->sensors.alarm_limits.pressure_min;
        float p_max = cfg->sensors.alarm_limits.pressure_max;
        float axis_kpa = PRESS_AXIS_FULLSCALE_KPA;
        float p_disp = pressure_to_disp(last->pressure_kpa, cfg->sensors.pressure_unit);
        float p_frac = clampf(last->pressure_kpa / axis_kpa, 0.f, 1.f);
        float safe_lo = clampf(p_min / axis_kpa, 0.f, 1.f);
        float safe_hi = clampf(p_max / axis_kpa, 0.f, 1.f);

        card_state_t pst = CARD_OK; const char *ptx = _t("NORMAL");
        if (last->pressure_kpa < p_min) { pst = CARD_ALARM; ptx = _t("BAJA"); }
        else if (last->pressure_kpa > p_max) { pst = CARD_ALARM; ptx = _t("ALTA"); }

        float pmn = have_mm ? pressure_to_disp(mn->pressure_kpa, cfg->sensors.pressure_unit) : p_disp;
        float pmx = have_mm ? pressure_to_disp(mx->pressure_kpa, cfg->sensors.pressure_unit) : p_disp;
        update_metric_card(&s_press, p_disp, p_frac, safe_lo, safe_hi, true, pst, ptx, pmn, pmx);

        char ab[24];
        snprintf(ab, sizeof(ab), "%.0f", pressure_to_disp(axis_kpa, cfg->sensors.pressure_unit));
        lv_label_set_text(s_press.ax_right, ab);
        lv_label_set_text(s_press.ax_mid, _t("seguro"));

        /* --- Flujo --- */
        float f_disp = flow_to_disp(last->flow_lpm, cfg->sensors.flow_unit);
        float f_axis = (cfg->sensors.flow_fullscale_lpm > 0.f) ? cfg->sensors.flow_fullscale_lpm
                                                               : FLOW_AXIS_FULLSCALE_LPM;
        float f_frac = clampf(last->flow_lpm / f_axis, 0.f, 1.f);
        card_state_t fst = CARD_OK; const char *ftx = _t("NORMAL");
        if (last->flow_lpm > f_axis * FLOW_HIGH_ZONE_FRAC) { fst = CARD_WARN; ftx = _t("CONSUMO ALTO"); }
        float fmn = have_mm ? flow_to_disp(mn->flow_lpm, cfg->sensors.flow_unit) : f_disp;
        float fmx = have_mm ? flow_to_disp(mx->flow_lpm, cfg->sensors.flow_unit) : f_disp;
        update_metric_card(&s_flow, f_disp, f_frac, 0.f, FLOW_HIGH_ZONE_FRAC, false, fst, ftx, fmn, fmx);
        char fb[24];
        snprintf(fb, sizeof(fb), "%.0f", flow_to_disp(f_axis, cfg->sensors.flow_unit));
        lv_label_set_text(s_flow.ax_right, fb);
        lv_label_set_text(s_flow.ax_mid, _t("alto"));

        /* --- Consumo acumulado del día --- */
        if (s_consumo_prev_ts >= 0 && last->ts_ms > s_consumo_prev_ts) {
            float dt_min = (float)(last->ts_ms - s_consumo_prev_ts) / 60000.f;
            /* descarta huecos > 5 min (ajuste de reloj, pausa larga del muestreo) */
            if (dt_min < 5.f && last->flow_lpm > 0.f)
                s_consumo_m3 += last->flow_lpm * dt_min / 1000.f;
        }
        s_consumo_prev_ts = last->ts_ms;
        if (s_consumo_val) {
            char cb[40];
            if (s_consumo_m3 < 0.1f)
                snprintf(cb, sizeof(cb), "%.0f %s", (double)(s_consumo_m3 * 1000.f), _t("L hoy"));
            else
                snprintf(cb, sizeof(cb), "%.2f %s", (double)s_consumo_m3, _t("m\xC2\xB3 hoy"));
            lv_label_set_text(s_consumo_val, cb);
        }
    }
}

#include "ui_cfg.h"
#include "ui_i18n.h"
#include "ui_theme.h"
#include "storage.h"
#include "screens/ui_mainScreen.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef ESP_PLATFORM
#include "display.h"   /* bsp_display_brightness_set() */
#include <time.h>      /* setenv/tzset */
#endif

/* ---------- helpers internos ---------- */
static void set_str(char *dst, size_t cap, const char *src)
{
    strncpy(dst, src, cap - 1);
    dst[cap - 1] = '\0';
}

static void save_and_refresh(AppConfig *cfg)
{
    (void)appcfg_save(cfg);        /* persiste en NVS */
    ui_main_apply_config(NULL);    /* refresca cabecera/gas/unidades en main */
}

/* ---------- setters ---------- */
void ui_cfg_set_pressure_unit(const char *unit)
{
    AppConfig *c = appcfg_cache_peek();
    if (!c || !unit) return;
    set_str(c->sensors.pressure_unit, sizeof(c->sensors.pressure_unit), unit);
    save_and_refresh(c);
}

void ui_cfg_set_flow_unit(const char *unit)
{
    AppConfig *c = appcfg_cache_peek();
    if (!c || !unit) return;
    set_str(c->sensors.flow_unit, sizeof(c->sensors.flow_unit), unit);
    save_and_refresh(c);
}

void ui_cfg_set_gas(const char *gas_key)
{
    AppConfig *c = appcfg_cache_peek();
    if (!c || !gas_key) return;
    set_str(c->sensors.gas_type, sizeof(c->sensors.gas_type), gas_key);
    save_and_refresh(c);
}

void ui_cfg_set_lang(const char *lang)
{
    AppConfig *c = appcfg_cache_peek();
    if (!c || !lang) return;
    set_str(c->general.lang, sizeof(c->general.lang), lang);
    save_and_refresh(c);   /* refresca los textos estáticos del main (_t) */
}

/* ---------- zona horaria ----------
 * Lista corta de zonas soportadas por la UI (IANA -> etiqueta -> POSIX TZ).
 * Offsets fijos (sin DST) para simplicidad del RTC. */
typedef struct { const char *iana; const char *label; const char *posix; } tz_opt_t;
static const tz_opt_t TZ_OPTS[] = {
    { "America/Bogota",              "GMT-5 Bogotá",  "<-05>5" },
    { "America/Mexico_City",         "GMT-6 CDMX",    "<-06>6" },
    { "America/Caracas",             "GMT-4 Caracas", "<-04>4" },
    { "America/Argentina/Buenos_Aires", "GMT-3 B.Aires", "<-03>3" },
    { "UTC",                         "GMT 0 UTC",     "UTC0" },
};
#define TZ_N ((int)(sizeof(TZ_OPTS) / sizeof(TZ_OPTS[0])))

int ui_cfg_tz_count(void) { return TZ_N; }
const char *ui_cfg_tz_label(int idx) { return TZ_OPTS[(idx >= 0 && idx < TZ_N) ? idx : 0].label; }

int ui_cfg_tz_index(void)
{
    const AppConfig *c = appcfg_cache_peek();
    if (c)
        for (int i = 0; i < TZ_N; i++)
            if (strcmp(c->general.timezone, TZ_OPTS[i].iana) == 0) return i;
    return 0; /* Bogotá por defecto */
}

void ui_cfg_apply_timezone(const char *iana)
{
#ifdef ESP_PLATFORM
    const char *posix = TZ_OPTS[0].posix;
    for (int i = 0; i < TZ_N; i++)
        if (iana && strcmp(iana, TZ_OPTS[i].iana) == 0) { posix = TZ_OPTS[i].posix; break; }
    setenv("TZ", posix, 1);
    tzset();
#else
    (void)iana; /* el sim usa la hora del PC */
#endif
}

void ui_cfg_set_tz_index(int idx)
{
    AppConfig *c = appcfg_cache_peek();
    if (!c || idx < 0 || idx >= TZ_N) return;
    set_str(c->general.timezone, sizeof(c->general.timezone), TZ_OPTS[idx].iana);
    (void)appcfg_save(c);
    ui_cfg_apply_timezone(TZ_OPTS[idx].iana);
}

/* ---------- edición de umbral con confirmación ---------- */

static int clamp_brightness(int pct)
{
    if (pct < 10)  return 10;
    if (pct > 100) return 100;
    return pct;
}

void ui_cfg_preview_brightness(int pct)
{
    pct = clamp_brightness(pct);
#ifdef ESP_PLATFORM
    bsp_display_brightness_set(pct);
#else
    (void)pct; /* el sim no tiene backlight */
#endif
}

void ui_cfg_set_brightness(int pct)
{
    AppConfig *c = appcfg_cache_peek();
    if (!c) return;
    c->general.brightness = clamp_brightness(pct);
    (void)appcfg_save(c);
    ui_cfg_preview_brightness(c->general.brightness);
}

bool ui_cfg_check_pin(const char *pin)
{
    const AppConfig *c = appcfg_cache_peek();
    if (!c || !pin) return false;
    return strcmp(pin, c->general.admin.pass) == 0;
}

#define UI_SESSION_MS (5U * 60U * 1000U)
static app_user_role_t s_auth_role = APP_ROLE_NONE;
static char s_auth_user[24] = "";
static uint32_t s_auth_until = 0;

int ui_auth_user_count(void) { const AppConfig *c=appcfg_cache_peek(); return c?c->general.users_count:0; }
const app_user_t *ui_auth_user_at(int index) { const AppConfig *c=appcfg_cache_peek(); return (!c||index<0||index>=c->general.users_count)?NULL:&c->general.users[index]; }
const app_user_t *ui_auth_factory(void) { const AppConfig *c=appcfg_cache_peek(); return c?&c->general.factory:NULL; }
bool ui_auth_login(const app_user_t *user, const char *pin)
{
    if (!user || !pin || user->role < APP_ROLE_TECH || strcmp(user->pin,pin)!=0) return false;
    s_auth_role=user->role; snprintf(s_auth_user,sizeof(s_auth_user),"%s",user->name);
    s_auth_until=lv_tick_get()+UI_SESSION_MS; return true;
}
void ui_auth_logout(void) { s_auth_role=APP_ROLE_NONE; s_auth_user[0]=0; s_auth_until=0; }
bool ui_auth_active(void)
{
    if(s_auth_role==APP_ROLE_NONE) return false;
    if((int32_t)(lv_tick_get()-s_auth_until)>=0){ui_auth_logout();return false;} return true;
}
app_user_role_t ui_auth_role(void){return ui_auth_active()?s_auth_role:APP_ROLE_NONE;}
const char *ui_auth_current_user(void){return ui_auth_active()?s_auth_user:"-";}

/* ---------- lecturas ---------- */
const char *ui_cfg_pressure_unit(void){ const AppConfig *c=appcfg_cache_peek(); return c?c->sensors.pressure_unit:"kpa"; }
const char *ui_cfg_flow_unit(void)    { const AppConfig *c=appcfg_cache_peek(); return c?c->sensors.flow_unit:"lpm"; }
const char *ui_cfg_gas(void)          { const AppConfig *c=appcfg_cache_peek(); return c?c->sensors.gas_type:"o2"; }
const char *ui_cfg_lang(void)         { const AppConfig *c=appcfg_cache_peek(); return c?c->general.lang:"es"; }
int  ui_cfg_brightness(void)          { const AppConfig *c=appcfg_cache_peek(); return c?c->general.brightness:80; }
const char *ui_cfg_theme(void){ const AppConfig *c=appcfg_cache_peek(); return c&&strcmp(c->general.theme,"light")==0?"light":"dark"; }
int ui_cfg_dim_minutes(void){ const AppConfig *c=appcfg_cache_peek(); return c?c->general.dim_minutes:5; }
void ui_cfg_set_dim_minutes(int m){ AppConfig*c=appcfg_cache_peek();if(!c)return;if(m!=0&&m!=1&&m!=5&&m!=10)m=5;c->general.dim_minutes=m;(void)appcfg_save(c); }
static lv_color_t visual_color(lv_color_t in,bool light)
{
    uint32_t x=lv_color_to_u32(in)&0xffffff;
    if(light){if(x==UI_C_SCREEN_BG)return ui_col(0xf3f5f7);if(x==UI_C_CARD_BG)return ui_col(0xffffff);if(x==UI_C_CARD_BG2)return ui_col(0xe9edf1);if(x==UI_C_BORDER)return ui_col(0xcbd2d9);if(x==UI_C_TEXT)return ui_col(0x18202a);if(x==UI_C_TEXT_2||x==UI_C_TEXT_3)return ui_col(0x53606d);if(x==UI_C_TEXT_MUTED)return ui_col(0x747f8a);}
    else {if(x==0xf3f5f7)return ui_col(UI_C_SCREEN_BG);if(x==0xffffff)return ui_col(UI_C_CARD_BG);if(x==0xe9edf1)return ui_col(UI_C_CARD_BG2);if(x==0xcbd2d9)return ui_col(UI_C_BORDER);if(x==0x18202a)return ui_col(UI_C_TEXT);if(x==0x53606d)return ui_col(UI_C_TEXT_2);if(x==0x747f8a)return ui_col(UI_C_TEXT_MUTED);}
    return in;
}
void ui_cfg_apply_visual_mode(lv_obj_t *o)
{
    if(!o) return;
    bool light=strcmp(ui_cfg_theme(),"light")==0;
    lv_obj_set_style_bg_color(o,visual_color(lv_obj_get_style_bg_color(o,LV_PART_MAIN),light),LV_PART_MAIN);
    lv_obj_set_style_text_color(o,visual_color(lv_obj_get_style_text_color(o,LV_PART_MAIN),light),LV_PART_MAIN);
    lv_obj_set_style_border_color(o,visual_color(lv_obj_get_style_border_color(o,LV_PART_MAIN),light),LV_PART_MAIN);
    uint32_t n=lv_obj_get_child_count(o);for(uint32_t i=0;i<n;i++)ui_cfg_apply_visual_mode(lv_obj_get_child(o,i));
}
void ui_cfg_set_theme(const char *theme){AppConfig*c=appcfg_cache_peek();if(!c)return;set_str(c->general.theme,sizeof(c->general.theme),theme&&strcmp(theme,"light")==0?"light":"dark");(void)appcfg_save(c);ui_cfg_apply_visual_mode(lv_screen_active());}
float ui_cfg_pressure_min(void)       { const AppConfig *c=appcfg_cache_peek(); return c?c->sensors.alarm_limits.pressure_min:0.f; }
float ui_cfg_pressure_max(void)       { const AppConfig *c=appcfg_cache_peek(); return c?c->sensors.alarm_limits.pressure_max:0.f; }
bool ui_cfg_pressure_limit_enabled(bool maximum)
{
    const AppConfig *c = appcfg_cache_peek();
    if (!c) return false;
    return maximum ? c->sensors.alarm_limits.pressure_max_enabled
                   : c->sensors.alarm_limits.pressure_min_enabled;
}
void ui_cfg_set_pressure_limit_enabled(bool maximum, bool enabled)
{
    AppConfig *c = appcfg_cache_peek();
    if (!c) return;
    if (maximum) c->sensors.alarm_limits.pressure_max_enabled = enabled;
    else         c->sensors.alarm_limits.pressure_min_enabled = enabled;
    save_and_refresh(c);
}

bool ui_cfg_alarm_tone(bool alert)
{
    const AppConfig *c = appcfg_cache_peek();
    return c ? (alert ? c->general.alarm.tone_alert : c->general.alarm.tone_warn) : false;
}
void ui_cfg_set_alarm_tone(bool alert, bool enabled)
{
    AppConfig *c = appcfg_cache_peek();
    if (!c) return;
    if (alert) c->general.alarm.tone_alert = enabled;
    else c->general.alarm.tone_warn = enabled;
    (void)appcfg_save(c);
}
int ui_cfg_alarm_volume(void){const AppConfig*c=appcfg_cache_peek();return c?c->general.alarm.volume:80;}
int ui_cfg_reannounce_minutes(void){const AppConfig*c=appcfg_cache_peek();return c?c->general.alarm.reannounce_minutes:15;}
int ui_cfg_max_silence_minutes(void){const AppConfig*c=appcfg_cache_peek();return c?c->general.alarm.max_silence_minutes:2;}
void ui_cfg_set_alarm_audio(int v,int r,int s){AppConfig*c=appcfg_cache_peek();if(!c)return;if(v<30)v=30;if(v>100)v=100;if(r<1)r=1;if(s<1)s=1;c->general.alarm.volume=v;c->general.alarm.reannounce_minutes=r;c->general.alarm.max_silence_minutes=s;(void)appcfg_save(c);}
bool ui_cfg_flow_limit_enabled(bool high){const AppConfig*c=appcfg_cache_peek();return c?(high?c->sensors.alarm_limits.flow_high_enabled:c->sensors.alarm_limits.flow_delta_enabled):false;}
float ui_cfg_flow_limit(bool high){const AppConfig*c=appcfg_cache_peek();return c?(high?c->sensors.alarm_limits.flow_high_limit:c->sensors.alarm_limits.flow_delta_threshold):0;}
int ui_cfg_flow_window_ms(void){const AppConfig*c=appcfg_cache_peek();return c?c->sensors.alarm_limits.flow_delta_window_ms:2000;}
void ui_cfg_set_flow_alarm(bool high,bool en,float v){AppConfig*c=appcfg_cache_peek();if(!c)return;if(v<1)v=1;if(high){c->sensors.alarm_limits.flow_high_enabled=en;c->sensors.alarm_limits.flow_high_limit=v;}else{c->sensors.alarm_limits.flow_delta_enabled=en;c->sensors.alarm_limits.flow_delta_threshold=v;}(void)appcfg_save(c);}
void ui_cfg_set_flow_window_ms(int ms){AppConfig*c=appcfg_cache_peek();if(!c)return;if(ms<500)ms=500;if(ms>10000)ms=10000;c->sensors.alarm_limits.flow_delta_window_ms=ms;(void)appcfg_save(c);}
/* ---------- conversión de unidades ----------
 * Internamente todo es kPa / L/min. El FW original convertía bar como kPa/10:
 * era un bug (1 bar = 100 kPa); aquí se corrige a /100. */
float ui_cfg_press_to_disp(float kpa)
{
    const char *u = ui_cfg_pressure_unit();
    if (strcmp(u, "psi") == 0) return kpa * 0.145038f;
    if (strcmp(u, "bar") == 0) return kpa / 100.0f;
    if (strcmp(u, "mpa") == 0) return kpa / 1000.0f;
    return kpa; /* kpa */
}

float ui_cfg_press_from_disp(float disp)
{
    const char *u = ui_cfg_pressure_unit();
    if (strcmp(u, "psi") == 0) return disp / 0.145038f;
    if (strcmp(u, "bar") == 0) return disp * 100.0f;
    if (strcmp(u, "mpa") == 0) return disp * 1000.0f;
    return disp; /* kpa */
}

float ui_cfg_flow_to_disp(float lpm)
{
    const char *u = ui_cfg_flow_unit();
    if (strcmp(u, "sccm") == 0) return lpm * 1000.0f;
    if (strcmp(u, "m3h") == 0)  return lpm * 0.06f;  /* 1 L/min = 0.06 m³/h */
    return lpm; /* lpm/slpm/nlpm */
}

void ui_cfg_press_fmt(char *buf, size_t cap, float disp)
{
    const char *u = ui_cfg_pressure_unit();
    /* bar/MPa quedan pequeños tras convertir: 1 decimal para no perder resolución */
    bool dec = (strcmp(u, "bar") == 0) || (strcmp(u, "mpa") == 0);
    snprintf(buf, cap, dec ? "%.1f" : "%.0f", (double)disp);
}

/* ---------- edición de umbral con confirmación ---------- */
static ui_edit_target_t s_target = UI_EDIT_NONE;
static float s_old = 0.f, s_new = 0.f;

void ui_edit_begin(ui_edit_target_t target)
{
    s_target = target;
    switch (target) {
    case UI_EDIT_PRES_MAX: s_old = ui_cfg_press_to_disp(ui_cfg_pressure_max()); break;
    case UI_EDIT_PRES_MIN: s_old = ui_cfg_press_to_disp(ui_cfg_pressure_min()); break;
    case UI_EDIT_FLOW_DELTA: s_old = ui_cfg_flow_to_disp(ui_cfg_flow_limit(false)); break;
    case UI_EDIT_FLOW_HIGH: s_old = ui_cfg_flow_to_disp(ui_cfg_flow_limit(true)); break;
    case UI_EDIT_AUDIO_REANNOUNCE: s_old = (float)ui_cfg_reannounce_minutes(); break;
    case UI_EDIT_AUDIO_SILENCE: s_old = (float)ui_cfg_max_silence_minutes(); break;
    default: s_old = 0.f; break;
    }
    s_new = s_old;
}

const char *ui_edit_label(void)
{
    switch (s_target) {
    case UI_EDIT_PRES_MAX: return _t("Presión - límite máximo");
    case UI_EDIT_PRES_MIN: return _t("Presión - límite mínimo");
    case UI_EDIT_FLOW_DELTA: return _t("Cambio abrupto de flujo");
    case UI_EDIT_FLOW_HIGH: return _t("Flujo alto sostenido");
    case UI_EDIT_AUDIO_REANNOUNCE: return _t("Reanunciar tras silencio");
    case UI_EDIT_AUDIO_SILENCE: return _t("Silencio máximo");
    default: return "";
    }
}
const char *ui_edit_unit(void)
{
    if (s_target == UI_EDIT_FLOW_DELTA || s_target == UI_EDIT_FLOW_HIGH) return ui_cfg_flow_unit();
    if (s_target == UI_EDIT_AUDIO_REANNOUNCE || s_target == UI_EDIT_AUDIO_SILENCE) return "min";
    return ui_cfg_pressure_unit();
}
float ui_edit_old(void) { return s_old; }
float ui_edit_new(void) { return s_new; }
void ui_edit_set_new(float v) { s_new = v; }
bool ui_edit_is_audio(void)
{
    return s_target == UI_EDIT_AUDIO_REANNOUNCE || s_target == UI_EDIT_AUDIO_SILENCE;
}
void ui_edit_apply(void)
{
    AppConfig *c = appcfg_cache_peek();
    if (!c || s_target == UI_EDIT_NONE) return;
    if (s_target == UI_EDIT_PRES_MAX || s_target == UI_EDIT_PRES_MIN) {
        float kpa = ui_cfg_press_from_disp(s_new);
        if (s_target == UI_EDIT_PRES_MAX) c->sensors.alarm_limits.pressure_max = kpa;
        else c->sensors.alarm_limits.pressure_min = kpa;
    } else if (s_target == UI_EDIT_FLOW_DELTA || s_target == UI_EDIT_FLOW_HIGH) {
        bool high = s_target == UI_EDIT_FLOW_HIGH;
        float lpm = s_new;
        if (strcmp(ui_cfg_flow_unit(), "sccm") == 0) lpm /= 1000.f;
        else if (strcmp(ui_cfg_flow_unit(), "m3h") == 0) lpm /= 0.06f;
        if (high) c->sensors.alarm_limits.flow_high_limit = lpm;
        else c->sensors.alarm_limits.flow_delta_threshold = lpm;
    } else if (s_target == UI_EDIT_AUDIO_REANNOUNCE) {
        c->general.alarm.reannounce_minutes = (int)s_new;
    } else if (s_target == UI_EDIT_AUDIO_SILENCE) {
        c->general.alarm.max_silence_minutes = (int)s_new;
    }
    save_and_refresh(c);
    s_target = UI_EDIT_NONE;
}

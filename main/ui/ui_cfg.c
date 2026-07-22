#include "ui_cfg.h"
#include "ui_i18n.h"
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

/* ---------- lecturas ---------- */
const char *ui_cfg_pressure_unit(void){ const AppConfig *c=appcfg_cache_peek(); return c?c->sensors.pressure_unit:"kpa"; }
const char *ui_cfg_flow_unit(void)    { const AppConfig *c=appcfg_cache_peek(); return c?c->sensors.flow_unit:"lpm"; }
const char *ui_cfg_gas(void)          { const AppConfig *c=appcfg_cache_peek(); return c?c->sensors.gas_type:"o2"; }
const char *ui_cfg_lang(void)         { const AppConfig *c=appcfg_cache_peek(); return c?c->general.lang:"es"; }
int  ui_cfg_brightness(void)          { const AppConfig *c=appcfg_cache_peek(); return c?c->general.brightness:80; }
float ui_cfg_pressure_min(void)       { const AppConfig *c=appcfg_cache_peek(); return c?c->sensors.alarm_limits.pressure_min:0.f; }
float ui_cfg_pressure_max(void)       { const AppConfig *c=appcfg_cache_peek(); return c?c->sensors.alarm_limits.pressure_max:0.f; }

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
    /* s_old/s_new viven en unidad de DISPLAY; a kPa solo al aplicar */
    s_old = ui_cfg_press_to_disp((target == UI_EDIT_PRES_MAX) ? ui_cfg_pressure_max()
                                                              : ui_cfg_pressure_min());
    s_new = s_old;
}

const char *ui_edit_label(void)
{
    return (s_target == UI_EDIT_PRES_MAX) ? _t("Presión · Umbral máximo")
                                          : _t("Presión · Umbral mínimo");
}
const char *ui_edit_unit(void) { return ui_cfg_pressure_unit(); }
float ui_edit_old(void) { return s_old; }
float ui_edit_new(void) { return s_new; }
void  ui_edit_set_new(float v) { s_new = v; }

void ui_edit_apply(void)
{
    AppConfig *c = appcfg_cache_peek();
    if (!c || s_target == UI_EDIT_NONE) return;
    float kpa = ui_cfg_press_from_disp(s_new);
    if (s_target == UI_EDIT_PRES_MAX) c->sensors.alarm_limits.pressure_max = kpa;
    else                              c->sensors.alarm_limits.pressure_min = kpa;
    save_and_refresh(c);
    s_target = UI_EDIT_NONE;
}

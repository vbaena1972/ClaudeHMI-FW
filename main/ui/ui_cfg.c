#include "ui_cfg.h"
#include "storage.h"
#include "screens/ui_mainScreen.h"
#include <string.h>

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
    (void)appcfg_save(c);
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
float ui_cfg_pressure_min(void)       { const AppConfig *c=appcfg_cache_peek(); return c?c->sensors.alarm_limits.pressure_min:0.f; }
float ui_cfg_pressure_max(void)       { const AppConfig *c=appcfg_cache_peek(); return c?c->sensors.alarm_limits.pressure_max:0.f; }

/* ---------- edición de umbral con confirmación ---------- */
static ui_edit_target_t s_target = UI_EDIT_NONE;
static float s_old = 0.f, s_new = 0.f;

void ui_edit_begin(ui_edit_target_t target)
{
    s_target = target;
    s_old = (target == UI_EDIT_PRES_MAX) ? ui_cfg_pressure_max() : ui_cfg_pressure_min();
    s_new = s_old;
}

const char *ui_edit_label(void)
{
    return (s_target == UI_EDIT_PRES_MAX) ? "Presión · Umbral máximo"
                                          : "Presión · Umbral mínimo";
}
const char *ui_edit_unit(void) { return ui_cfg_pressure_unit(); }
float ui_edit_old(void) { return s_old; }
float ui_edit_new(void) { return s_new; }
void  ui_edit_set_new(float v) { s_new = v; }

void ui_edit_apply(void)
{
    AppConfig *c = appcfg_cache_peek();
    if (!c || s_target == UI_EDIT_NONE) return;
    if (s_target == UI_EDIT_PRES_MAX) c->sensors.alarm_limits.pressure_max = s_new;
    else                              c->sensors.alarm_limits.pressure_min = s_new;
    save_and_refresh(c);
    s_target = UI_EDIT_NONE;
}

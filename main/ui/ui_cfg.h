#ifndef UI_CFG_H
#define UI_CFG_H

/* Puente entre las pantallas de configuración y el modelo AppConfig en NVS.
 * Patrón: appcfg_cache_peek() -> modificar -> appcfg_save() -> refrescar main. */

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Conversión de unidades (según la unidad configurada) ---
 * Internamente TODO se guarda en kPa (presión) y L/min (flujo);
 * estas funciones convierten solo para mostrar/editar. */
float ui_cfg_press_to_disp(float kpa);
float ui_cfg_press_from_disp(float disp);
float ui_cfg_flow_to_disp(float lpm);
void  ui_cfg_press_fmt(char *buf, size_t cap, float disp); /* decimales según unidad */

/* --- Setters directos (guardan en NVS y refrescan la pantalla principal) --- */
void ui_cfg_set_pressure_unit(const char *unit);  /* "psi"|"bar"|"kpa"|"mpa" */
void ui_cfg_set_flow_unit(const char *unit);      /* "lpm"|"sccm"|"m3h"|...  */
void ui_cfg_set_gas(const char *gas_key);         /* "o2"|"air_med"|"n2o"|"vac" */
void ui_cfg_set_lang(const char *lang);           /* "es"|"en" */

/* Brillo de pantalla (10..100 %).
 * preview: aplica al backlight sin persistir (para arrastre del slider).
 * set:     aplica + guarda en NVS (al soltar el slider). */
void ui_cfg_preview_brightness(int pct);
void ui_cfg_set_brightness(int pct);

/* --- Zona horaria (lista corta soportada por la UI) --- */
int         ui_cfg_tz_count(void);
const char *ui_cfg_tz_label(int idx);       /* "GMT-5 Bogotá" */
int         ui_cfg_tz_index(void);          /* índice de la zona configurada */
void        ui_cfg_set_tz_index(int idx);   /* guarda + setenv TZ/tzset */
void        ui_cfg_apply_timezone(const char *iana); /* aplica sin guardar (boot) */

/* Valida un PIN de 4 dígitos contra general.admin.pass. */
bool ui_cfg_check_pin(const char *pin);

/* Lecturas para poblar las pantallas al cargarlas. */
const char *ui_cfg_pressure_unit(void);
const char *ui_cfg_flow_unit(void);
const char *ui_cfg_gas(void);
const char *ui_cfg_lang(void);
int  ui_cfg_brightness(void);
float ui_cfg_pressure_min(void);
float ui_cfg_pressure_max(void);

/* --- Edición de umbral con confirmación (keypad -> confirmar -> PIN -> aplicar) --- */
typedef enum {
    UI_EDIT_NONE = 0,
    UI_EDIT_PRES_MIN,
    UI_EDIT_PRES_MAX,
} ui_edit_target_t;

void        ui_edit_begin(ui_edit_target_t target); /* fija target y valor "antes" */
const char *ui_edit_label(void);                    /* p.ej. "Presión · Umbral máximo" */
const char *ui_edit_unit(void);                     /* unidad de presión actual */
float       ui_edit_old(void);                      /* en unidad de DISPLAY */
float       ui_edit_new(void);                      /* en unidad de DISPLAY */
void        ui_edit_set_new(float v);               /* en unidad de DISPLAY */
void        ui_edit_apply(void);                    /* convierte a kPa, NVS + refresca main */

#ifdef __cplusplus
}
#endif

#endif /* UI_CFG_H */

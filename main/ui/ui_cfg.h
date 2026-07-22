#ifndef UI_CFG_H
#define UI_CFG_H

/* Puente entre las pantallas de configuración y el modelo AppConfig en NVS.
 * Patrón: appcfg_cache_peek() -> modificar -> appcfg_save() -> refrescar main. */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Setters directos (guardan en NVS y refrescan la pantalla principal) --- */
void ui_cfg_set_pressure_unit(const char *unit);  /* "psi"|"bar"|"kpa"|"mpa" */
void ui_cfg_set_flow_unit(const char *unit);      /* "lpm"|"sccm"|"m3h"|...  */
void ui_cfg_set_gas(const char *gas_key);         /* "o2"|"air_med"|"n2o"|"vac" */
void ui_cfg_set_lang(const char *lang);           /* "es"|"en" */

/* Valida un PIN de 4 dígitos contra general.admin.pass. */
bool ui_cfg_check_pin(const char *pin);

/* Lecturas para poblar las pantallas al cargarlas. */
const char *ui_cfg_pressure_unit(void);
const char *ui_cfg_flow_unit(void);
const char *ui_cfg_gas(void);
const char *ui_cfg_lang(void);
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
float       ui_edit_old(void);
float       ui_edit_new(void);
void        ui_edit_set_new(float v);
void        ui_edit_apply(void);                    /* escribe en NVS + refresca main */

#ifdef __cplusplus
}
#endif

#endif /* UI_CFG_H */

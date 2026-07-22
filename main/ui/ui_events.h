#ifndef UI_EVENTS_H
#define UI_EVENTS_H

/* Handlers de eventos de configuración (cargar/guardar AppConfig, provisioning, etc.).
 * La lógica real se porta desde el HMI anterior a medida que se agregan las pantallas
 * de configuración (Fases 4-5). Este header declara las firmas conforme se implementan. */

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* (Se irán descomentando/añadiendo por fase)
 * void ui_general_cfg_load_to_widgets(lv_event_t *e);
 * void ui_general_cfg_save_from_widgets(lv_event_t *e);
 * void ui_sensor_cfg_load_to_widgets(lv_event_t *e);
 * void ui_sensor_cfg_save_from_widgets(lv_event_t *e);
 * void ui_wifi_cfg_load_to_widgets(lv_event_t *e);
 * ...
 */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* UI_EVENTS_H */

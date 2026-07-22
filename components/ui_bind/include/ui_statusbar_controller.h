#pragma once
#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Inicializa el controlador de la barra de estado.
   Crea un timer LVGL que observa cambios de link/conn (WiFi/ETH/BLE/Cloud)
   y refresca los íconos aplicando prioridad Ethernet > WiFi. */
void ui_statusbar_controller_init(void);
void ui_statusbar_controller_deinit(void);

/* Fuerza un refresco inmediato (el worker tomará el snapshot runtime).
   Útil tras start/stop de WiFi, cambios de link ETH, BLE connect/disconnect, etc. */
void ui_statusbar_request_refresh(void);

/* NUEVO: Fija la visibilidad (A/B) de los íconos según la configuración (NVS).
   Llamar UNA VEZ al cargar ui_mainScreen (p. ej. en LV_EVENT_SCREEN_LOADED),
   después de leer AppConfig desde NVS.
   Esto evita leer NVS en cada refresh de la barra de estado. */
void ui_statusbar_set_enabled(bool wifi, bool eth, bool bt, bool cloud);

#ifdef __cplusplus
}
#endif

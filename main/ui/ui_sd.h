#ifndef UI_SD_H
#define UI_SD_H

/* Overlay modal de mantenimiento por microSD (AppConfig + certificados AWS).
 * Lo maneja sd_monitor_task en main.c, siempre bajo bsp_display_lock.
 * Todas las funciones son seguras si el overlay no existe (no-op). */

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_sd_show(void);                                /* crea el overlay en layer_top */
void ui_sd_progress(int pct, const char *status);     /* barra 0..100 + texto de etapa */
void ui_sd_finish_restart(lv_event_cb_t restart_cb);  /* muestra "Aplicar y reiniciar" */
void ui_sd_close(void);                               /* cierra el overlay (sin cambios) */

#ifdef __cplusplus
}
#endif

#endif /* UI_SD_H */

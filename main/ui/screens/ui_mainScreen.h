#ifndef UI_MAINSCREEN_H
#define UI_MAINSCREEN_H

#include "lvgl.h"
#include "storage.h"          /* AppConfig */
#include "sensors_runtime.h"  /* sensor_sample_t */
#include "alarm_mgr.h"        /* alarm_clinical_state_t */

#ifdef __cplusplus
extern "C" {
#endif

/* --- Ciclo de vida --- */
void ui_mainScreen_screen_init(void);
void ui_mainScreen_screen_destroy(void);

/* --- Pantalla y widgets de estado (consumidos por components/ui_bind) --- */
extern lv_obj_t *ui_mainScreen;
extern lv_obj_t *ui_statusMainComm;      /* contenedor de la fila de iconos */
extern lv_obj_t *ui_bluetoothStatusMain; /* label icono BT   */
extern lv_obj_t *ui_wifiStatusMain;      /* label icono WiFi */
extern lv_obj_t *ui_ethernetStatusMain;  /* label icono ETH  */
extern lv_obj_t *ui_cloudStatusMain;     /* label icono nube */
extern lv_obj_t *ui_alarmBtnMain;        /* banner de estado (táctil = mute) */

/* --- API de binding (la llama ui_refresh_task en main.c bajo bsp_display_lock) --- */
void ui_main_update(const sensor_sample_t *last, bool have_last,
                    const sensor_sample_t *mn, const sensor_sample_t *mx, bool have_mm,
                    const AppConfig *cfg,
                    alarm_clinical_state_t state, bool muted);

/* Refresca cabecera/gas/marca desde AppConfig (llamar on-load). */
void ui_main_apply_config(const AppConfig *cfg);

/* Fija el reloj de la cabecera ("14:32"). */
void ui_main_set_clock(const char *hhmm);

#ifdef __cplusplus
}
#endif

#endif /* UI_MAINSCREEN_H */

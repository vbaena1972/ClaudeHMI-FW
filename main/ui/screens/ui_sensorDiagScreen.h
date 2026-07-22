#ifndef UI_SENSORDIAGSCREEN_H
#define UI_SENSORDIAGSCREEN_H
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
extern lv_obj_t *ui_sensorDiagScreen;
void ui_sensorDiagScreen_screen_init(void);
void ui_sensorDiagScreen_screen_destroy(void);
#ifdef __cplusplus
}
#endif
#endif

#ifndef UI_SENSOREDITSCREEN_H
#define UI_SENSOREDITSCREEN_H
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
extern lv_obj_t *ui_sensorEditScreen;
void ui_sensorEditScreen_screen_init(void);
void ui_sensorEditScreen_screen_destroy(void);
#ifdef __cplusplus
}
#endif
#endif

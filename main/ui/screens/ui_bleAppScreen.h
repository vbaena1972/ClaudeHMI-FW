#ifndef UI_BLEAPPSCREEN_H
#define UI_BLEAPPSCREEN_H
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
extern lv_obj_t *ui_bleAppScreen;
void ui_bleAppScreen_screen_init(void);
void ui_bleAppScreen_screen_destroy(void);
#ifdef __cplusplus
}
#endif
#endif

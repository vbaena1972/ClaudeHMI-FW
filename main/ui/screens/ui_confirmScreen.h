#ifndef UI_CONFIRMSCREEN_H
#define UI_CONFIRMSCREEN_H
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
extern lv_obj_t *ui_confirmScreen;
void ui_confirmScreen_screen_init(void);
void ui_confirmScreen_screen_destroy(void);
#ifdef __cplusplus
}
#endif
#endif

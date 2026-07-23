#ifndef UI_LOGINSCREEN_H
#define UI_LOGINSCREEN_H
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
extern lv_obj_t *ui_loginScreen;
void ui_loginScreen_screen_init(void);
void ui_loginScreen_screen_destroy(void);
#ifdef __cplusplus
}
#endif
#endif
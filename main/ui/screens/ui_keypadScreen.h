#ifndef UI_KEYPADSCREEN_H
#define UI_KEYPADSCREEN_H
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
extern lv_obj_t *ui_keypadScreen;
void ui_keypadScreen_screen_init(void);
void ui_keypadScreen_screen_destroy(void);
#ifdef __cplusplus
}
#endif
#endif

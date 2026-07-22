#ifndef UI_CONNECTIVITYSCREEN_H
#define UI_CONNECTIVITYSCREEN_H
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
extern lv_obj_t *ui_connectivityScreen;
void ui_connectivityScreen_screen_init(void);
void ui_connectivityScreen_screen_destroy(void);
#ifdef __cplusplus
}
#endif
#endif

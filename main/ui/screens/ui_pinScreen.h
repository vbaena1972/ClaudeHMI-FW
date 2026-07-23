#ifndef UI_PINSCREEN_H
#define UI_PINSCREEN_H
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
extern lv_obj_t *ui_pinScreen;
void ui_pinScreen_screen_init(void);
void ui_pinScreen_set_config_entry(bool enabled);
void ui_pinScreen_screen_destroy(void);
#ifdef __cplusplus
}
#endif
#endif

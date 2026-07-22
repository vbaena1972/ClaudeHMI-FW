#ifndef UI_NETBLESCREEN_H
#define UI_NETBLESCREEN_H
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
extern lv_obj_t *ui_netBleScreen;
void ui_netBleScreen_screen_init(void);
void ui_netBleScreen_screen_destroy(void);
#ifdef __cplusplus
}
#endif
#endif

#ifndef UI_NETWIFISCREEN_H
#define UI_NETWIFISCREEN_H
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
extern lv_obj_t *ui_netWifiScreen;
void ui_netWifiScreen_screen_init(void);
void ui_netWifiScreen_screen_destroy(void);
#ifdef __cplusplus
}
#endif
#endif

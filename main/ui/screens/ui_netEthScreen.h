#ifndef UI_NETETHSCREEN_H
#define UI_NETETHSCREEN_H
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
extern lv_obj_t *ui_netEthScreen;
void ui_netEthScreen_screen_init(void);
void ui_netEthScreen_screen_destroy(void);
#ifdef __cplusplus
}
#endif
#endif

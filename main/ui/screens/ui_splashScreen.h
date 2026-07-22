#ifndef UI_SPLASHSCREEN_H
#define UI_SPLASHSCREEN_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern lv_obj_t *ui_splashScreen;

void ui_splashScreen_screen_init(void);
void ui_splashScreen_screen_destroy(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_SPLASHSCREEN_H */

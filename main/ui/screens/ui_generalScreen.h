#ifndef UI_GENERALSCREEN_H
#define UI_GENERALSCREEN_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern lv_obj_t *ui_generalScreen;

void ui_generalScreen_screen_init(void);
void ui_generalScreen_screen_destroy(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_GENERALSCREEN_H */

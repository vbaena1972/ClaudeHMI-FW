#ifndef UI_INFOSCREEN_H
#define UI_INFOSCREEN_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern lv_obj_t *ui_infoScreen;

void ui_infoScreen_screen_init(void);
void ui_infoScreen_screen_destroy(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_INFOSCREEN_H */

#ifndef UI_NAV_H
#define UI_NAV_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Navegación entre pantallas con pila de retroceso.
 * Las pantallas se crean una vez y se conservan (no se destruyen al salir). */

void ui_nav_init(lv_obj_t *root);          /* fija la pantalla raíz (main), sin cargarla */
void ui_nav_show_root(void);               /* carga la raíz (tras el splash) */
void ui_nav_load(lv_obj_t *scr);           /* carga scr y apila la actual  */
void ui_nav_replace(lv_obj_t *scr);        /* carga scr SIN apilar (splash->main) */
void ui_nav_back(void);                    /* vuelve a la anterior          */
lv_obj_t *ui_nav_current(void);

/* Callback listo para usar en LV_EVENT_CLICKED de botones "atrás". */
void ui_nav_back_event_cb(lv_event_t *e);

#ifdef __cplusplus
}
#endif

#endif /* UI_NAV_H */

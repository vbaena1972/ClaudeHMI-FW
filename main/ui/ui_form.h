#ifndef UI_FORM_H
#define UI_FORM_H

/* Infraestructura de formularios de configuración (textarea + teclado en pantalla,
 * switch, dropdown) con el estilo del tema. La usan las pantallas de red. */

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Crea una pantalla de formulario: header (back + título + botón "Guardar") + un
 * contenedor de contenido con scroll + un teclado en pantalla (oculto).
 * Devuelve la pantalla; *out_content recibe la columna donde agregar campos. */
lv_obj_t *ui_form_begin(const char *title, lv_obj_t **out_content, lv_event_cb_t save_cb);

/* Campo de texto: tarjeta con caption + textarea de una línea. Devuelve el textarea.
 * Si password=true, oculta el texto. */
lv_obj_t *ui_form_textarea(lv_obj_t *parent, const char *caption, const char *value, bool password);

/* Fila con etiqueta + switch. Devuelve el switch (usar lv_obj_has_state(sw, LV_STATE_CHECKED)). */
lv_obj_t *ui_form_switch(lv_obj_t *parent, const char *label, const char *sub, bool on);

/* Fila con caption + dropdown. opts en el formato de LVGL ("A\nB\nC"). Devuelve el dropdown. */
lv_obj_t *ui_form_dropdown(lv_obj_t *parent, const char *caption, const char *opts, int selected);

/* Fila de dos campos de texto lado a lado (p.ej. IP + máscara). */
lv_obj_t *ui_form_row2(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif

#endif /* UI_FORM_H */

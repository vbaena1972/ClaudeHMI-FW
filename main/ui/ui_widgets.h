#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

/* Fábricas de widgets compartidas para la nueva HMI.
 * Encapsulan el estilo del tema (ui_theme.h) para no repetir código en cada pantalla. */

#include "lvgl.h"
#include "ui_theme.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Crea una pantalla base: fondo del tema, sin scroll, sin padding. */
lv_obj_t *ui_screen_base(void);

/* Contenedor "tarjeta": fondo UI_C_CARD_BG, borde sutil, esquinas redondeadas. */
lv_obj_t *ui_card(lv_obj_t *parent);

/* Contenedor plano (sin fondo/borde), sin scroll, sin padding. Útil para filas/columnas. */
lv_obj_t *ui_box(lv_obj_t *parent);

/* Label de texto con fuente y color del tema. */
lv_obj_t *ui_label(lv_obj_t *parent, const char *txt, const lv_font_t *font, uint32_t color_hex);

/* Label que usa una fuente de iconos Tabler (ui_icons.h). Igual que ui_label pero semántico. */
lv_obj_t *ui_icon(lv_obj_t *parent, const char *sym, const lv_font_t *font, uint32_t color_hex);

/* Píldora/badge redondeada con texto (p. ej. "NORMAL", "GMT-5"). */
lv_obj_t *ui_pill(lv_obj_t *parent, const char *txt, const lv_font_t *font,
                  uint32_t text_hex, uint32_t bg_hex, uint32_t border_hex);

/* Cuadro de icono con fondo (el recuadro de marca del header, 30x30). */
lv_obj_t *ui_icon_badge(lv_obj_t *parent, const char *sym, const lv_font_t *font,
                        uint32_t icon_hex, uint32_t bg_hex, lv_coord_t size);

/* Header simple con flecha de retroceso + título + subtítulo.
 * Devuelve el contenedor del header; *out_back recibe el botón de back (clickable). */
lv_obj_t *ui_header_back(lv_obj_t *parent, const char *title, const char *subtitle,
                         lv_obj_t **out_back);

/* Header estándar de pantalla: fila flex space-between con [back(28) + título] a la
 * izquierda (back ya cableado a ui_nav_back). Devuelve el header para que el llamador
 * agregue contenido a la derecha (badge/pill). */
lv_obj_t *ui_nav_header(lv_obj_t *parent, const char *title);

/* Fila de menú tipo "ajustes": icono + título + subtítulo + valor + chevron.
 * Devuelve la fila (clickable). *out_value recibe el label del valor (puede ser NULL). */
lv_obj_t *ui_menu_row(lv_obj_t *parent, const char *sym, const char *title,
                      const char *subtitle, const char *value, lv_obj_t **out_value);

/* Aplica estilo de "botón" del tema a un objeto (bg + radio + estado pressed). */
void ui_style_button(lv_obj_t *obj, uint32_t bg_hex);

/* Utilidad: quitar scroll y padding de un objeto. */
void ui_kill_scroll(lv_obj_t *obj);

#ifdef __cplusplus
}
#endif

#endif /* UI_WIDGETS_H */

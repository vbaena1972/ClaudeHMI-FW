#ifndef UI_I18N_H
#define UI_I18N_H

/* i18n mínimo es/en.
 * Los literales de las pantallas quedan en español y se envuelven en _t():
 * con lang=="en" se busca la traducción en la tabla de ui_i18n.c; si no
 * existe (o lang=="es") se devuelve el literal tal cual — degradación
 * elegante: una cadena sin traducir simplemente se ve en español.
 * Las pantallas se recrean al abrirlas (fresh_* en ui.c), así que el cambio
 * de idioma aplica al navegar sin reiniciar. */

#ifdef __cplusplus
extern "C" {
#endif

const char *_t(const char *es);

#ifdef __cplusplus
}
#endif

#endif /* UI_I18N_H */

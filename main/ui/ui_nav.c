#include "ui_nav.h"
#include "ui_cfg.h"

#define UI_NAV_STACK_MAX 8

static lv_obj_t *s_stack[UI_NAV_STACK_MAX];
static int s_top = -1;   /* índice de la pantalla actual dentro del stack */

void ui_nav_init(lv_obj_t *root)
{
    /* Registra 'root' como base de la pila. NO carga la pantalla: el llamador
     * decide qué mostrar primero (p. ej. el splash antes que main). */
    s_top = -1;
    if (root) s_stack[++s_top] = root;
}

void ui_nav_show_root(void)
{
    if (s_top >= 0)
        lv_screen_load_anim(s_stack[0], LV_SCR_LOAD_ANIM_FADE_IN, 250, 0, false);
    s_top = 0;
}

lv_obj_t *ui_nav_current(void)
{
    return (s_top >= 0) ? s_stack[s_top] : NULL;
}

void ui_nav_load(lv_obj_t *scr)
{
    if (!scr) return;
    if (s_top < UI_NAV_STACK_MAX - 1) {
        s_stack[++s_top] = scr;
    } else {
        /* stack lleno: desplaza y reemplaza el tope */
        for (int i = 0; i < UI_NAV_STACK_MAX - 1; i++) s_stack[i] = s_stack[i + 1];
        s_stack[s_top] = scr;
    }
    ui_cfg_apply_visual_mode(scr);
    lv_screen_load_anim(scr, LV_SCR_LOAD_ANIM_MOVE_LEFT, 180, 0, false);
}

void ui_nav_replace(lv_obj_t *scr)
{
    if (!scr) return;
    if (s_top < 0) s_top = 0;
    s_stack[s_top] = scr;
    ui_cfg_apply_visual_mode(scr);
    lv_screen_load_anim(scr, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);
}

void ui_nav_back(void)
{
    if (s_top <= 0) return;   /* ya en la raíz */
    s_top--;
    lv_screen_load_anim(s_stack[s_top], LV_SCR_LOAD_ANIM_MOVE_RIGHT, 180, 0, false);
}

void ui_nav_pop_to(lv_obj_t *scr)
{
    if (!scr) { ui_nav_back(); return; }
    /* Busca scr en la pila (desde arriba) y hace pop hasta dejarla en el tope. */
    for (int i = s_top; i >= 0; i--) {
        if (s_stack[i] == scr) {
            s_top = i;
            lv_screen_load_anim(scr, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
            return;
        }
    }
    ui_nav_back();  /* no está en la pila: retroceso simple */
}

void ui_nav_back_event_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_back();
}

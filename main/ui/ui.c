#include "ui.h"
#include "storage.h"
#include "ui_statusbar_controller.h"

/* En el dispositivo LV_COLOR_DEPTH=16 (ST7796/RGB565, fijado por sdkconfig).
 * El simulador de PC usa 32bpp; solo cambia la precisión de color, no el layout. */

/* Pantallas secundarias: se RECREAN al abrirlas hacia adelante (fresh).
 * Es seguro: al abrir X, X nunca está en la pila de navegación (solo sus
 * ancestros), así que destruir la instancia vieja no deja punteros colgando.
 * Beneficios: datos de config siempre frescos y el cambio de idioma (_t)
 * aplica al navegar, sin reiniciar. */
static lv_obj_t *get_general(void)
{
    if (ui_generalScreen) ui_generalScreen_screen_destroy();
    ui_generalScreen_screen_init();
    return ui_generalScreen;
}
static lv_obj_t *get_info(void)
{
    if (ui_infoScreen) ui_infoScreen_screen_destroy();
    ui_infoScreen_screen_init();
    return ui_infoScreen;
}
static lv_obj_t *get_general_simple(void)
{
    if (ui_generalSimpleScreen) ui_generalSimpleScreen_screen_destroy();
    ui_generalSimpleScreen_screen_init();
    return ui_generalSimpleScreen;
}
static lv_obj_t *get_connectivity(void)
{
    if (ui_connectivityScreen) ui_connectivityScreen_screen_destroy();
    ui_connectivityScreen_screen_init();
    return ui_connectivityScreen;
}
static lv_obj_t *get_sensor_edit(void)
{
    if (ui_sensorEditScreen) ui_sensorEditScreen_screen_destroy();
    ui_sensorEditScreen_screen_init();
    return ui_sensorEditScreen;
}
static lv_obj_t *get_sensor_diag(void)
{
    if (ui_sensorDiagScreen) ui_sensorDiagScreen_screen_destroy();
    ui_sensorDiagScreen_screen_init();
    return ui_sensorDiagScreen;
}
static lv_obj_t *get_bleapp(void)
{
    if (ui_bleAppScreen) ui_bleAppScreen_screen_destroy();
    ui_bleAppScreen_screen_init();
    return ui_bleAppScreen;
}
/* Los diálogos (keypad/confirm/pin) se recrean cada vez: mantienen estado interno
 * (valor tecleado, dígitos del PIN) que debe resetearse al abrirlos. */
static lv_obj_t *fresh_keypad(void)
{
    if (ui_keypadScreen) ui_keypadScreen_screen_destroy();
    ui_keypadScreen_screen_init();
    return ui_keypadScreen;
}
static lv_obj_t *fresh_confirm(void)
{
    if (ui_confirmScreen) ui_confirmScreen_screen_destroy();
    ui_confirmScreen_screen_init();
    return ui_confirmScreen;
}
static lv_obj_t *fresh_pin(void)
{
    if (ui_pinScreen) ui_pinScreen_screen_destroy();
    ui_pinScreen_screen_init();
    return ui_pinScreen;
}
/* Formularios de red: se recrean al abrir para cargar la config actual. */
static lv_obj_t *fresh_net_wifi(void)  { if (ui_netWifiScreen)  ui_netWifiScreen_screen_destroy();  ui_netWifiScreen_screen_init();  return ui_netWifiScreen; }
static lv_obj_t *fresh_net_eth(void)   { if (ui_netEthScreen)   ui_netEthScreen_screen_destroy();   ui_netEthScreen_screen_init();   return ui_netEthScreen; }
static lv_obj_t *fresh_net_cloud(void) { if (ui_netCloudScreen) ui_netCloudScreen_screen_destroy(); ui_netCloudScreen_screen_init(); return ui_netCloudScreen; }
static lv_obj_t *fresh_net_ble(void)   { if (ui_netBleScreen)   ui_netBleScreen_screen_destroy();   ui_netBleScreen_screen_init();   return ui_netBleScreen; }

/* Fin del splash -> transición a la pantalla principal.
 * auto_del=true deja que LVGL libere el splash al terminar la animación
 * (evita borrarlo a mano en medio de la transición). */
static void splash_done_cb(lv_timer_t *t)
{
    (void)t;
    lv_screen_load_anim(ui_mainScreen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, true);
    ui_splashScreen = NULL;   /* será liberado por LVGL; main ya es la raíz (s_top=0) */
}

void ui_init(void)
{
    /* Tema base oscuro para controles no estilizados (dropdowns, teclados, switches). */
    lv_display_t *dispp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp,
                                              lv_color_hex(UI_C_OK),
                                              lv_color_hex(UI_C_BLUE),
                                              true /*dark*/,
                                              UI_FONT_MD);
    lv_display_set_theme(dispp, theme);

    /* Pantalla principal (raíz de navegación) */
    ui_mainScreen_screen_init();
    ui_main_apply_config(NULL);

    const AppConfig *cfg = appcfg_cache_peek();
    if (cfg)
        ui_statusbar_set_enabled(cfg->wifi.enabled, cfg->eth.enabled,
                                 cfg->bt.enabled, cfg->cloud.enabled);

    ui_nav_init(ui_mainScreen);   /* registra main como raíz (no la carga aún) */

    /* Splash de arranque -> main tras ~2.2 s */
    ui_splashScreen_screen_init();
    lv_screen_load(ui_splashScreen);
    lv_timer_t *t = lv_timer_create(splash_done_cb, 2200, NULL);
    lv_timer_set_repeat_count(t, 1);
}

void ui_destroy(void)
{
    ui_mainScreen_screen_destroy();
}

/* ============ Dispatch de navegación ============ */
void ui_open_general_cb(lv_event_t *e)        { (void)e; ui_nav_load(get_general()); }
void ui_open_general_simple_cb(lv_event_t *e) { (void)e; ui_nav_load(get_general_simple()); }
void ui_open_info_cb(lv_event_t *e)           { (void)e; ui_nav_load(get_info()); }
void ui_open_sensor_cb(lv_event_t *e)         { (void)e; ui_nav_load(get_sensor_edit()); }
void ui_open_sensordiag_cb(lv_event_t *e)     { (void)e; ui_nav_load(get_sensor_diag()); }
void ui_open_connectivity_cb(lv_event_t *e)   { (void)e; ui_nav_load(get_connectivity()); }
void ui_open_bleapp_cb(lv_event_t *e)         { (void)e; ui_nav_load(get_bleapp()); }
void ui_open_keypad_cb(lv_event_t *e)         { (void)e; ui_nav_load(fresh_keypad()); }
void ui_open_confirm_cb(lv_event_t *e)        { (void)e; ui_nav_load(fresh_confirm()); }
void ui_open_pin_cb(lv_event_t *e)            { (void)e; ui_nav_load(fresh_pin()); }
void ui_open_net_wifi_cb(lv_event_t *e)       { (void)e; ui_nav_load(fresh_net_wifi()); }
void ui_open_net_eth_cb(lv_event_t *e)        { (void)e; ui_nav_load(fresh_net_eth()); }
void ui_open_net_cloud_cb(lv_event_t *e)      { (void)e; ui_nav_load(fresh_net_cloud()); }
void ui_open_net_ble_cb(lv_event_t *e)        { (void)e; ui_nav_load(fresh_net_ble()); }

#ifndef UI_H
#define UI_H

/* Orquestador de la nueva HMI (Axira / Claude Design).
 * Reemplaza el ui.h generado por SquareLine Studio. */

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "ui_theme.h"
#include "ui_icons.h"
#include "ui_widgets.h"
#include "ui_nav.h"
#include "ui_events.h"

/* Pantallas */
#include "screens/ui_mainScreen.h"
#include "screens/ui_splashScreen.h"
#include "screens/ui_generalScreen.h"
#include "screens/ui_generalSimpleScreen.h"
#include "screens/ui_infoScreen.h"
#include "screens/ui_connectivityScreen.h"
#include "screens/ui_sensorEditScreen.h"
#include "screens/ui_sensorDiagScreen.h"
#include "screens/ui_keypadScreen.h"
#include "screens/ui_confirmScreen.h"
#include "screens/ui_pinScreen.h"
#include "screens/ui_loginScreen.h"
#include "screens/ui_datetimeScreen.h"
#include "screens/ui_bleAppScreen.h"
#include "screens/ui_netWifiScreen.h"
#include "screens/ui_netEthScreen.h"
#include "screens/ui_netCloudScreen.h"
#include "screens/ui_netBleScreen.h"

/* --- Ciclo de vida (llamado por main.c) --- */
void ui_init(void);
void ui_destroy(void);

/* --- Dispatch de navegación (callbacks LVGL usados por botones) ---
 * Cada pantalla se crea perezosamente la primera vez que se abre. */
void ui_open_general_cb(lv_event_t *e);
void ui_open_general_authenticated_cb(lv_event_t *e);
void ui_open_general_simple_cb(lv_event_t *e);
void ui_open_datetime_cb(lv_event_t *e);
void ui_open_sensor_cb(lv_event_t *e);        /* sensorScreen editable */
void ui_open_sensordiag_cb(lv_event_t *e);    /* diagnóstico de sensores */
void ui_open_info_cb(lv_event_t *e);
void ui_open_connectivity_cb(lv_event_t *e);
void ui_open_bleapp_cb(lv_event_t *e);
void ui_open_pin_cb(lv_event_t *e);
void ui_open_config_pin_cb(lv_event_t *e);
void ui_open_login_cb(lv_event_t *e);
void ui_open_keypad_cb(lv_event_t *e);
void ui_open_confirm_cb(lv_event_t *e);
void ui_open_net_wifi_cb(lv_event_t *e);
void ui_open_net_eth_cb(lv_event_t *e);
void ui_open_net_cloud_cb(lv_event_t *e);
void ui_open_net_ble_cb(lv_event_t *e);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* UI_H */

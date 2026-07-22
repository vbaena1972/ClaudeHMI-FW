#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** Inicializa el icono WiFi en una imagen LVGL ya creada (por ejemplo ui_WifiIcon).
     *  Se registrará al wifi_mgr para recibir cambios de estado. */
    void ui_wifi_main_icon_init(lv_obj_t *img_obj);
    void ui_wifi_main_icon_deinit(void);
    /** Conversión pública RSSI (dBm) -> barras (1..4) para reutilizar en otras pantallas */
    int ui_wifi_rssi_to_bars(int rssi_dbm);

#ifdef __cplusplus
}
#endif

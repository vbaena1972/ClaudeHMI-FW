#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_netif.h"
#include "storage.h"

    typedef enum
    {
        WIFI_STATUS_OFF = 0,
        WIFI_STATUS_CONNECTING,
        WIFI_STATUS_CONNECTED,
        WIFI_STATUS_DISCONNECTED,
    } wifi_status_t;

    typedef void (*wifi_status_cb_t)(wifi_status_t status, int rssi_dbm);

    typedef struct
    {
        bool connected;
        char ssid[33];
        int rssi_dbm;
        char ip[16];
        char mask[16];
        char gw[16];
        char dns1[16];
        char dns2[16];
    } wifi_netinfo_t;

    esp_err_t wifi_mgr_init(void);
    esp_err_t wifi_mgr_start(void);
    esp_err_t wifi_mgr_stop(void);

    esp_err_t wifi_mgr_apply_from_cache(void);

    esp_err_t wifi_mgr_get_netinfo(wifi_netinfo_t *out);
    void wifi_mgr_set_status_cb(wifi_status_cb_t cb);
    bool wifi_mgr_is_enabled(void);
    bool wifi_mgr_has_ip(void);

    esp_netif_t *wifi_mgr_get_netif(void);

    esp_err_t wifi_mgr_update_credentials(const char *ssid, const char *password);

#ifdef __cplusplus
}
#endif

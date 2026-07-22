#include "esp_event.h"
#include "esp_log.h"
#include "esp_eth.h"
#include "esp_wifi.h"

// Cabeceras que el compilador no encontraba
#include "net_core.h"
#include "state_pub.h"
#include "transport_mqtt.h"
#include "time_mgr.h"

static const char *TAG = "net_core";

static void on_ip_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == IP_EVENT)
    {
        if (id == IP_EVENT_ETH_GOT_IP)
        {
            ip_event_got_ip_t *e = (ip_event_got_ip_t *)data;
            ESP_LOGI(TAG, "ETH obtuvo IP: " IPSTR, IP2STR(&e->ip_info.ip));
            state_on_net_available(true);

            // Llamamos a buscar la hora atómica (SNTP)
            time_mgr_start_sntp();
        }
        else if (id == IP_EVENT_STA_GOT_IP)
        {
            ip_event_got_ip_t *e = (ip_event_got_ip_t *)data;
            ESP_LOGI(TAG, "Wi-Fi obtuvo IP: " IPSTR, IP2STR(&e->ip_info.ip));
            state_on_net_available(true);

            // Llamamos a buscar la hora atómica (SNTP)
            time_mgr_start_sntp();
        }
    }
}

static void on_eth_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (id == ETHERNET_EVENT_DISCONNECTED)
    {
        ESP_LOGW(TAG, "ETH link down");
        state_on_net_available(false);
        transport_mqtt_on_net_down();
    }
}

static void on_wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGW(TAG, "Wi-Fi link down");
        state_on_net_available(false);
        transport_mqtt_on_net_down();
    }
}

// Inicializador del Orquestador
esp_err_t net_core_init(void)
{
    // Escuchar cuando tengamos Internet
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &on_ip_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_ip_event, NULL));

    // Escuchar cuando perdamos el enlace físico
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &on_eth_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_event, NULL));

    ESP_LOGI(TAG, "Orquestador de red inicializado");
    return ESP_OK;
}
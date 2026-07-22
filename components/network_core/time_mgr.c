#include "time_mgr.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include "rtc_rv3028.h"
#include <sys/time.h>
#include "storage.h"
#include "transport_mqtt.h" // Para avisar a MQTT

static const char *TAG = "time_mgr";
static bool s_time_valid = false;

// Callback que llama ESP-IDF cuando SNTP logra sincronizar la hora
static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Sincronización SNTP completada con éxito.");
    s_time_valid = true;

    // Tomamos la hora recién sincronizada del ESP32
    time_t now = 0;
    struct tm timeinfo = { 0 };
    time(&now);
    localtime_r(&now, &timeinfo);

    // Actualizamos el RTC RV-3028-C7 de hardware
    if (rtc_rv3028_set_time(&timeinfo) == ESP_OK) {
        ESP_LOGI(TAG, "RTC RV-3028-C7 actualizado: %04d-%02d-%02d %02d:%02d:%02d",
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }

    // ¡Notificamos a MQTT que ya es seguro conectar!
    transport_mqtt_on_time_ready(); 
}

void time_mgr_init(void)
{
    // Configuramos la zona horaria de Colombia por defecto
    setenv("TZ", "COT5", 1);
    tzset();

    // Intentamos cargar la hora desde el RTC RV-3028
    struct tm rtc_time = {0};
    if (rtc_rv3028_get_time(&rtc_time) == ESP_OK) {
        // Validamos si el RTC tiene un año coherente (ej. > 2023)
        if ((rtc_time.tm_year + 1900) >= 2024) {
            struct timeval tv;
            tv.tv_sec = mktime(&rtc_time);
            tv.tv_usec = 0;
            settimeofday(&tv, NULL);
            ESP_LOGI(TAG, "Hora del sistema restaurada desde RTC: %04d-%02d-%02d %02d:%02d",
                     rtc_time.tm_year + 1900, rtc_time.tm_mon + 1, rtc_time.tm_mday,
                     rtc_time.tm_hour, rtc_time.tm_min);
            s_time_valid = true;
        } else {
            ESP_LOGW(TAG, "El RTC tiene una fecha antigua o descalibrada. Esperando a SNTP.");
        }
    }
}

void time_mgr_start_sntp(void)
{
    ESP_LOGI(TAG, "Iniciando servicio SNTP con redundancia...");

    // 1. Limpiamos cualquier instancia previa por si el Wi-Fi se reconectó
    esp_netif_sntp_deinit();

    // 2. Configuramos el servidor principal
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    config.sync_cb = time_sync_notification_cb;
    
    // 3. Inicializamos el motor SNTP
    esp_netif_sntp_init(&config);

    // 4. INYECCIÓN CRÍTICA: Añadimos servidores de respaldo de alta disponibilidad
    esp_sntp_setservername(1, "time.google.com");
    esp_sntp_setservername(2, "time.nist.gov");
    
    ESP_LOGI(TAG, "Servidores NTP configurados. Esperando sincronización...");
}

bool time_mgr_is_time_valid(void)
{
    return s_time_valid;
}
#include "rtc_rv3028.h"
#include "esp_log.h"

static const char *TAG = "rv3028";
#define RV3028_ADDR 0x52

static i2c_master_dev_handle_t s_rtc_dev = NULL;

// Funciones auxiliares BCD a Decimal y viceversa
static uint8_t bcd2dec(uint8_t val) { return ((val / 16 * 10) + (val % 16)); }
static uint8_t dec2bcd(uint8_t val) { return ((val / 10 * 16) + (val % 10)); }

esp_err_t rtc_rv3028_init(i2c_master_bus_handle_t bus_handle)
{
    if (!bus_handle)
        return ESP_ERR_INVALID_ARG;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = RV3028_ADDR,
        .scl_speed_hz = 100000, // 100 kHz estándar
    };

    esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &s_rtc_dev);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error añadiendo dispositivo RTC al bus I2C: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "RTC RV-3028-C7 inicializado en bus I2C");
    return ESP_OK;
}

esp_err_t rtc_rv3028_get_time(struct tm *timeinfo)
{
    if (!s_rtc_dev || !timeinfo)
        return ESP_ERR_INVALID_ARG;

    uint8_t reg = 0x00;
    uint8_t data[7];

    // Timeout explícito de 100ms en lugar de -1
    esp_err_t err = i2c_master_transmit_receive(s_rtc_dev, &reg, 1, data, 7, 100);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error leyendo hora del RTC. Bus ocupado o chip desconectado.");
        return err;
    }

    timeinfo->tm_sec = bcd2dec(data[0] & 0x7F);
    timeinfo->tm_min = bcd2dec(data[1] & 0x7F);
    timeinfo->tm_hour = bcd2dec(data[2] & 0x3F);
    timeinfo->tm_wday = bcd2dec(data[3] & 0x07);
    timeinfo->tm_mday = bcd2dec(data[4] & 0x3F);
    timeinfo->tm_mon = bcd2dec(data[5] & 0x1F) - 1;
    timeinfo->tm_year = bcd2dec(data[6]) + 100;

    return ESP_OK;
}

esp_err_t rtc_rv3028_set_time(const struct tm *timeinfo)
{
    if (!s_rtc_dev || !timeinfo)
        return ESP_ERR_INVALID_ARG;

    uint8_t data[8];
    data[0] = 0x00; // Registro inicial
    data[1] = dec2bcd(timeinfo->tm_sec);
    data[2] = dec2bcd(timeinfo->tm_min);
    data[3] = dec2bcd(timeinfo->tm_hour);
    data[4] = dec2bcd(timeinfo->tm_wday);
    data[5] = dec2bcd(timeinfo->tm_mday);
    data[6] = dec2bcd(timeinfo->tm_mon + 1);
    data[7] = dec2bcd(timeinfo->tm_year - 100);

    esp_err_t err = i2c_master_transmit(s_rtc_dev, data, 8, -1);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error escribiendo hora al RTC");
    }
    return err;
}
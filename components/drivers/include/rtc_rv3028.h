#pragma once

#include <stdbool.h>
#include <time.h>
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

// Inicializa el dispositivo I2C para el RTC en el bus existente
esp_err_t rtc_rv3028_init(i2c_master_bus_handle_t bus_handle);

// Lee la hora del RTC y la guarda en la estructura 'tm'
esp_err_t rtc_rv3028_get_time(struct tm *timeinfo);

// Establece la hora del RTC basada en la estructura 'tm'
esp_err_t rtc_rv3028_set_time(const struct tm *timeinfo);

#ifdef __cplusplus
}
#endif
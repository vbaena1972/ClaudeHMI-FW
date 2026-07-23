#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Estados de Alarma segÃƒÂºn prioridad NFPA 99
typedef enum {
    ALARM_STATE_NORMAL = 0,
    ALARM_STATE_WARNING,    // PrecauciÃƒÂ³n (Amarillo) - Fuera de rango ÃƒÂ³ptimo
    ALARM_STATE_ALERT       // CrÃƒÂ­tico (Rojo) - PresiÃƒÂ³n extrema o falla de lÃƒÂ­nea
} alarm_clinical_state_t;

// Inicializa el perifÃƒÂ©rico PWM (LEDC) para el Buzzer
esp_err_t alarm_mgr_init(int buzzer_gpio);

// Procesador central: EvalÃƒÂºa los sensores y maneja los timings de Mute
void alarm_mgr_process(float current_pressure, float current_flow, uint32_t sensor_faults);

// AcciÃƒÂ³n fÃƒÂ­sica al presionar el botÃƒÂ³n de MUTE del HMI
void alarm_mgr_press_mute(void);
void alarm_mgr_test_buzzer(void);

// Getters para que la pantalla (UI) sepa de quÃƒÂ© color pintarse
alarm_clinical_state_t alarm_mgr_get_current_state(void);
bool alarm_mgr_is_muted(void);
uint32_t alarm_mgr_get_sensor_faults(void);

#ifdef __cplusplus
}
#endif
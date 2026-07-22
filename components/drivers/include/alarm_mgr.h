#pragma once
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Estados de Alarma según prioridad NFPA 99
typedef enum {
    ALARM_STATE_NORMAL = 0,
    ALARM_STATE_WARNING,    // Precaución (Amarillo) - Fuera de rango óptimo
    ALARM_STATE_ALERT       // Crítico (Rojo) - Presión extrema o falla de línea
} alarm_clinical_state_t;

// Inicializa el periférico PWM (LEDC) para el Buzzer
esp_err_t alarm_mgr_init(int buzzer_gpio);

// Procesador central: Evalúa los sensores y maneja los timings de Mute
void alarm_mgr_process(float current_pressure, float current_flow);

// Acción física al presionar el botón de MUTE del HMI
void alarm_mgr_press_mute(void);

// Getters para que la pantalla (UI) sepa de qué color pintarse
alarm_clinical_state_t alarm_mgr_get_current_state(void);
bool alarm_mgr_is_muted(void);

#ifdef __cplusplus
}
#endif
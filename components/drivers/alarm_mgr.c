#include "alarm_mgr.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "storage.h"

static const char *TAG = "alarm_mgr";

// Parámetros de PWM adaptados para el Driver PAM8906
#define LEDC_SPEED_MODE         LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_DUTY_RES           LEDC_TIMER_10_BIT
#define BUZZER_FREQ_HZ          2000    // Resonancia óptima para máxima presión sonora
#define BUZZER_DUTY_50_PERCENT  512     // 50% de 1024 para onda cuadrada perfecta

// Variables operacionales de control
static alarm_clinical_state_t s_current_state = ALARM_STATE_NORMAL;
static bool s_is_muted = false;
static int64_t s_mute_end_time_us = 0;
static int64_t s_last_toggle_time_us = 0;
static bool s_beep_active = false;

// Variables para el análisis matemático diferencial de flujo (Delta)
static float s_baseline_flow = 0.0f;
static int64_t s_baseline_time_us = 0;

esp_err_t alarm_mgr_init(int buzzer_gpio)
{
    ESP_LOGI(TAG, "Inicializando PWM para Driver PAM8906 en GPIO %d...", buzzer_gpio);

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_SPEED_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = BUZZER_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_SPEED_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = buzzer_gpio,
        .duty           = 0, 
        .hpoint         = 0
    };
    return ledc_channel_config(&ledc_channel);
}

static void set_buzzer_acoustic(bool turn_on)
{
    // El PAM8906 amplifica la señal cuadrada diferencial cuando detecta pulsos en DIN
    uint32_t duty = turn_on ? BUZZER_DUTY_50_PERCENT : 0;
    ledc_set_duty(LEDC_SPEED_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_SPEED_MODE, LEDC_CHANNEL);
}

void alarm_mgr_process(float current_pressure, float current_flow)
{
    // 1. Carga rápida desde la caché o NVS protegida
    AppConfig cfg;
    appcfg_defaults(&cfg);
    appcfg_load(&cfg);

    int64_t now = esp_timer_get_time();

    // --- EVALUACIÓN DE PRESIÓN ---
    float p_min = cfg.sensors.alarm_limits.pressure_min;
    float p_max = cfg.sensors.alarm_limits.pressure_max;

    // --- EVALUACIÓN DE FLUJO (Análisis de Delta temporal) ---
    float flow_delta_thresh = cfg.sensors.alarm_limits.flow_delta_threshold;
    int flow_window_ms = cfg.sensors.alarm_limits.flow_delta_window_ms;
    bool flow_warning = false;

    // Si la ventana de tiempo ya caducó o es el primer arranque, tomamos una nueva línea base de flujo
    if (s_baseline_time_us == 0 || (now - s_baseline_time_us) >= (flow_window_ms * 1000ULL)) {
        s_baseline_flow = current_flow;
        s_baseline_time_us = now;
    } else {
        // Evaluamos si el flujo tuvo un pico o caída abrupta dentro de la ventana de tiempo actual
        float delta = current_flow - s_baseline_flow;
        if (delta < 0) delta = -delta; // Convertimos a valor absoluto para detectar cambios hacia arriba o abajo

        if (delta > flow_delta_thresh) {
            flow_warning = true;
        }
    }

    // 2. Clasificación del peor escenario según norma hospitalaria
    alarm_clinical_state_t target_state = ALARM_STATE_NORMAL;

    // Escenario crítico (ALERT - Rojo): Variación severa de presión fuera de rangos vitales
    if (current_pressure < (p_min - 15.0f) || current_pressure > (p_max + 15.0f)) {
        target_state = ALARM_STATE_ALERT;
    } 
    // Escenario moderado (WARNING - Amarillo): Cruce de límites de presión O variación súbita de flujo (Fuga)
    else if (current_pressure < p_min || current_pressure > p_max || flow_warning) {
        target_state = ALARM_STATE_WARNING;
    }

    // 3. Cláusula de Seguridad NFPA 99: Si pasa de Warning a Alert, se anula el Mute
    if (target_state == ALARM_STATE_ALERT && s_current_state == ALARM_STATE_WARNING) {
        if (s_is_muted) {
            ESP_LOGW(TAG, "¡Condición médica escaló a CRÍTICA! Rompiendo Mute automáticamente.");
            s_is_muted = false;
        }
    }

    s_current_state = target_state;

    // 4. Evaluar fin del tiempo de silencio (Mute Timeout)
    if (s_is_muted && now >= s_mute_end_time_us) {
        ESP_LOGI(TAG, "Tiempo de Mute vencido. Reactivando señal sonora.");
        s_is_muted = false;
    }

    // Si todo está normal o el sistema está silenciado, apagamos el PWM de inmediato
    if (s_current_state == ALARM_STATE_NORMAL || s_is_muted) {
        set_buzzer_acoustic(false);
        return;
    }

    // 5. Cadencia del Beep según Severidad
    // Alert: Ráfagas muy rápidas (150ms encendido / 150ms apagado)
    // Warning: Ráfagas lentas y espaciadas (500ms encendido / 500ms apagado)
    int64_t toggle_interval_us = (s_current_state == ALARM_STATE_ALERT) ? 150000 : 500000;

    if (now - s_last_toggle_time_us >= toggle_interval_us) {
        s_beep_active = !s_beep_active;
        s_last_toggle_time_us = now;
        
        // Verificamos si las banderas en NVS tienen el sonido activo para este nivel
        bool tone_allowed = (s_current_state == ALARM_STATE_ALERT) ? cfg.general.alarm.tone_alert : cfg.general.alarm.tone_warn;
        
        set_buzzer_acoustic(s_beep_active && tone_allowed);
    }
}

void alarm_mgr_press_mute(void)
{
    if (s_current_state == ALARM_STATE_NORMAL) return;

    AppConfig cfg;
    appcfg_defaults(&cfg);
    appcfg_load(&cfg);

    // Extraemos el tiempo exacto que el usuario configuró desde AWS o SD
    int timeout_seconds = (s_current_state == ALARM_STATE_ALERT) ? cfg.general.alarm.alert_timeout_s : cfg.general.alarm.warn_timeout_s;

    s_is_muted = true;
    s_mute_end_time_us = esp_timer_get_time() + ((int64_t)timeout_seconds * 1000000ULL);

    ESP_LOGW(TAG, "Buzzer silenciado vía software por %d segundos.", timeout_seconds);
    set_buzzer_acoustic(false); // Apagado instantáneo
}

alarm_clinical_state_t alarm_mgr_get_current_state(void) { return s_current_state; }
bool alarm_mgr_is_muted(void) { return s_is_muted; }
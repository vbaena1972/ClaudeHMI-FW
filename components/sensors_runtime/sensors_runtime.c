#include "sensors_runtime.h"

#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_log.h"

//------------------------------------------------------------------
// ParÃ¡metros del buffer y tarea dummy
//------------------------------------------------------------------

#define SENSORS_BUFFER_LEN 600      // ~600 muestras
#define SENSORS_DUMMY_PERIOD_MS 50 // 5 Hz (para pruebas)
#define SENSORS_TASK_STACK 4096
#define SENSORS_TASK_PRIO 5

static const char *TAG = "sensors_runtime";

//------------------------------------------------------------------
// Estado interno
//------------------------------------------------------------------

static sensor_sample_t s_buf[SENSORS_BUFFER_LEN];
static size_t s_head = 0;  // prÃ³xima posiciÃ³n de escritura
static size_t s_count = 0; // nÂº de elementos vÃ¡lidos en buffer

static SemaphoreHandle_t s_lock = NULL;

// Config cachÃ© (por si luego quieres usar cal.* aquÃ­)
static AppConfig s_cfg_cache;
static bool s_cfg_valid = false;
static uint32_t s_reported_faults = SENSOR_FAULT_NONE;

// Tarea dummy (opcional)
static TaskHandle_t s_dummy_task_handle = NULL;

static void sensors_dummy_task(void *arg);

//------------------------------------------------------------------
// Helpers de sincronizaciÃ³n
//------------------------------------------------------------------

static inline void lock(void)
{
    if (s_lock)
        xSemaphoreTake(s_lock, portMAX_DELAY);
}

static inline void unlock(void)
{
    if (s_lock)
        xSemaphoreGive(s_lock);
}

//------------------------------------------------------------------
// API pÃºblica
//------------------------------------------------------------------

bool sensors_runtime_init(const AppConfig *cfg)
{
    ESP_LOGI(TAG, "Iniciando sensors_runtime...");
    if (!s_lock)
    {
        s_lock = xSemaphoreCreateMutex();
        if (!s_lock)
        {
            ESP_LOGI(TAG, "No se pudo crear mutex");
            return false;
        }
    }

    lock();
    s_head = 0;
    s_count = 0;

    if (cfg)
    {
        memcpy(&s_cfg_cache, cfg, sizeof(AppConfig));
        s_cfg_valid = true;
    }
    else
    {
        memset(&s_cfg_cache, 0, sizeof(AppConfig));
        s_cfg_valid = false;
    }
    unlock();

    // Arrancamos la tarea dummy de generaciÃ³n de muestras (para pruebas).
    // Cuando tengas lectura real, puedes comentar esto y empujar tÃº mismo.
    if (!s_dummy_task_handle)
    {
        BaseType_t ok = xTaskCreatePinnedToCore(
            /* pxTaskCode   */ sensors_dummy_task,
            /* pcName       */ "sensors_dummy",
            /* usStackDepth */ SENSORS_TASK_STACK,
            /* pvParameters */ NULL,
            /* uxPriority   */ SENSORS_TASK_PRIO,
            /* pxCreatedTask*/ &s_dummy_task_handle,
            /* xCoreID      */ tskNO_AFFINITY);
        if (ok != pdPASS)
        {
            ESP_LOGI(TAG, "No se pudo crear tarea dummy de sensores");
            s_dummy_task_handle = NULL;
            // No devolvemos false porque el runtime puede servir igual
            // si tÃº empujas muestras manualmente.
        }
    }

    ESP_LOGI(TAG, "Init ok (dummy task %s)",
             s_dummy_task_handle ? "ON" : "OFF");
    return true;
}

void sensors_runtime_update_config(const AppConfig *cfg)
{
    if (!cfg)
        return;

    lock();
    memcpy(&s_cfg_cache, cfg, sizeof(AppConfig));
    s_cfg_valid = true;
    unlock();

    ESP_LOGI(TAG, "Config actualizada en sensors_runtime");
}

void sensors_runtime_push_sample(const sensor_sample_t *s)
{
    if (!s)
        return;

    lock();

    s_buf[s_head] = *s;
    s_head = (s_head + 1) % SENSORS_BUFFER_LEN;
    if (s_count < SENSORS_BUFFER_LEN)
        s_count++;

    unlock();
}

void sensors_runtime_report_faults(uint32_t faults)
{
    lock(); s_reported_faults = faults; unlock();
}

uint32_t sensors_runtime_get_faults(void)
{
    uint32_t faults; sensor_sample_t last = {0}; bool have = false; bool cfg_ok;
    lock();
    faults = s_reported_faults; cfg_ok = s_cfg_valid;
    if (s_count) { size_t i=(s_head+SENSORS_BUFFER_LEN-1)%SENSORS_BUFFER_LEN; last=s_buf[i]; have=true; }
    unlock();
    if (!cfg_ok) faults |= SENSOR_FAULT_CONFIG;
    if (!have) return faults | SENSOR_FAULT_NO_DATA;
    int64_t age_ms = esp_timer_get_time()/1000 - last.ts_ms;
    if (age_ms < 0 || age_ms > 1000) faults |= SENSOR_FAULT_STALE;
    float pmax = 1379.0f * 1.10f;
    float fmax = s_cfg_cache.sensors.flow_fullscale_lpm > 0 ? s_cfg_cache.sensors.flow_fullscale_lpm * 1.20f : 120.0f;
    if (!isfinite(last.pressure_kpa) || !isfinite(last.flow_lpm) || last.pressure_kpa < -5.f || last.pressure_kpa > pmax || last.flow_lpm < -1.f || last.flow_lpm > fmax) faults |= SENSOR_FAULT_INVALID;
    return faults;
}
bool sensors_runtime_get_last(sensor_sample_t *out)
{
    if (!out)
        return false;

    lock();

    if (s_count == 0)
    {
        unlock();
        return false;
    }

    size_t last_idx = (s_head + SENSORS_BUFFER_LEN - 1) % SENSORS_BUFFER_LEN;
    *out = s_buf[last_idx];

    unlock();
    return true;
}

bool sensors_runtime_get_min_max(int64_t window_ms,
                                 sensor_sample_t *min_out,
                                 sensor_sample_t *max_out)
{
    if (window_ms <= 0)
        return false;

    lock();

    if (s_count == 0)
    {
        unlock();
        return false;
    }

    int64_t now_ms = esp_timer_get_time() / 1000;
    int64_t from_ms = now_ms - window_ms;

    bool has = false;
    sensor_sample_t min_s = {0}, max_s = {0};

    // Recorremos desde la muestra mÃ¡s reciente hacia atrÃ¡s
    for (size_t i = 0; i < s_count; ++i)
    {
        size_t idx = (s_head + SENSORS_BUFFER_LEN - 1 - i) % SENSORS_BUFFER_LEN;
        const sensor_sample_t *s = &s_buf[idx];

        if (s->ts_ms < from_ms)
            break; // mÃ¡s allÃ¡ de la ventana

        if (!has)
        {
            min_s = max_s = *s;
            has = true;
        }
        else
        {
            // PresiÃ³n
            if (s->pressure_kpa < min_s.pressure_kpa)
                min_s.pressure_kpa = s->pressure_kpa;
            if (s->pressure_kpa > max_s.pressure_kpa)
                max_s.pressure_kpa = s->pressure_kpa;

            // Flujo
            if (s->flow_lpm < min_s.flow_lpm)
                min_s.flow_lpm = s->flow_lpm;
            if (s->flow_lpm > max_s.flow_lpm)
                max_s.flow_lpm = s->flow_lpm;
        }
    }

    unlock();

    if (!has)
        return false;

    if (min_out)
        *min_out = min_s;
    if (max_out)
        *max_out = max_s;

    return true;
}

bool sensors_runtime_get_series(int64_t window_ms,
                                sensor_sample_t *out,
                                size_t max,
                                size_t *out_count)
{
    if (!out || max == 0 || window_ms <= 0)
        return false;

    lock();

    if (s_count == 0)
    {
        unlock();
        if (out_count)
            *out_count = 0;
        return false;
    }

    int64_t now_ms = esp_timer_get_time() / 1000;
    int64_t from_ms = now_ms - window_ms;

    // Primero contamos cuÃ¡ntos entran en la ventana (desde el mÃ¡s antiguo hacia arriba)
    // Para devolverlos en orden cronolÃ³gico ascendente.
    size_t n = 0;
    sensor_sample_t tmp[SENSORS_BUFFER_LEN]; // buffer temporal en stack (16 bytes * 600 = ~9.6 kB)

    size_t idx = (s_head + SENSORS_BUFFER_LEN - s_count) % SENSORS_BUFFER_LEN; // Ã­ndice del mÃ¡s antiguo
    for (size_t i = 0; i < s_count; ++i)
    {
        const sensor_sample_t *s = &s_buf[idx];

        if (s->ts_ms >= from_ms)
        {
            if (n < SENSORS_BUFFER_LEN)
            {
                tmp[n++] = *s;
            }
        }

        idx = (idx + 1) % SENSORS_BUFFER_LEN;
    }

    if (n == 0)
    {
        unlock();
        if (out_count)
            *out_count = 0;
        return false;
    }

    // Copiamos hasta max elementos al buffer de salida
    size_t copy_n = (n < max) ? n : max;
    memcpy(out, tmp, copy_n * sizeof(sensor_sample_t));

    unlock();

    if (out_count)
        *out_count = copy_n;

    return true;
}

//------------------------------------------------------------------
// Tarea dummy de sensores (para pruebas de UI)
//------------------------------------------------------------------

static void sensors_dummy_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "Dummy task iniciada (genera muestras simuladas)");

    float p = 0.0f; // kPa inicial
    float f = 0.0f;  // L/min inicial
    float dp = 1.25f;  // (Antes 5.0f)
    float df = 0.50f;  // (Antes 2.0f)

    while (1)
    {
        int64_t now_ms = esp_timer_get_time() / 1000;

        sensor_sample_t s = {
            .ts_ms = now_ms,
            .pressure_kpa = p,
            .flow_lpm = f,
        };

        sensors_runtime_push_sample(&s);

        // PequeÃ±a evoluciÃ³n de prueba (diente de sierra)
        p += dp;
        if (p > 1400.0f || p < 0.0f)
            dp = -dp;

        f += df;
        if (f > 100.0f || f < 0.0f)
            df = -df;
        ESP_LOGI(TAG, "Dummy sample pushed: P=%.2f kPa, F=%.2f L/min", p, f);
        vTaskDelay(pdMS_TO_TICKS(SENSORS_DUMMY_PERIOD_MS));
    }
}

bool sensors_runtime_get_aws_telemetry(int64_t window_ms,
                                       float *p_min, float *p_max, float *p_avg,
                                       float *f_min, float *f_max, float *f_avg)
{
    if (!s_lock) return false;

    int64_t now_ms = esp_timer_get_time() / 1000;
    int64_t start_ms = now_ms - window_ms;

    xSemaphoreTake(s_lock, portMAX_DELAY);

    if (s_count == 0) {
        xSemaphoreGive(s_lock);
        return false;
    }

    float pmin = 999999.0f, pmax = -999999.0f, psum = 0.0f;
    float fmin = 999999.0f, fmax = -999999.0f, fsum = 0.0f;
    int valid_samples = 0;

    // Recorremos el buffer circular buscando muestras en la ventana de tiempo
    for (size_t i = 0; i < s_count; i++) {
        size_t idx = (s_head + SENSORS_BUFFER_LEN - s_count + i) % SENSORS_BUFFER_LEN;
        
        if (s_buf[idx].ts_ms >= start_ms) {
            float p = s_buf[idx].pressure_kpa;
            float f = s_buf[idx].flow_lpm;

            if (p < pmin) pmin = p;
            if (p > pmax) pmax = p;
            psum += p;

            if (f < fmin) fmin = f;
            if (f > fmax) fmax = f;
            fsum += f;

            valid_samples++;
        }
    }
    xSemaphoreGive(s_lock);

    if (valid_samples == 0) return false;

    // Asignamos resultados
    if (p_min) *p_min = pmin;
    if (p_max) *p_max = pmax;
    if (p_avg) *p_avg = psum / valid_samples;

    if (f_min) *f_min = fmin;
    if (f_max) *f_max = fmax;
    if (f_avg) *f_avg = fsum / valid_samples;

    return true;
}
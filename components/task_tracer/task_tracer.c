// task_tracer.c
//
// Muestrea el estado de todas las tareas FreeRTOS usando los contadores de
// runtime que ya estan habilitados (CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y).
//
// Formato de salida por serie (filtrable con Python):
//   TRACE,<ts_ms>,<nombre>,<estado>,<cpu_pct_x10>,<stack_hwm_words>,<core>
//   TRACE_END,<ts_ms>
//
// cpu_pct_x10: CPU% * 10 para evitar floats (ej. 125 = 12.5%).
// En dual-core el maximo teorico es 2000 (200%), porque dos tareas pueden
// correr simultaneamente.  Los IDLE suelen ser ~400-800 total (40-80% libre).
//
// Limitaciones vs Percepio/SystemView:
//   - Resolucion = interval_ms, no microsegundos por context-switch.
//   - No captura ISR.
//   - Suficiente para detectar: CPU hog, starvation, leaks de stack,
//     tareas fantasma, deadlocks (tarea siempre en BLK con CPU=0).

#include "task_tracer.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

#define TAG             "TRACER"
#define MAX_TASKS       32

// Entrada de seguimiento por tarea (identificada por handle)
typedef struct {
    TaskHandle_t     handle;
    unsigned long    last_runtime;
    char             name[configMAX_TASK_NAME_LEN];
    uint8_t          valid;
} track_t;

static TaskStatus_t      s_status[MAX_TASKS];
static track_t           s_track[MAX_TASKS];
static uint32_t          s_track_n  = 0;
static unsigned long     s_last_total = 0;
static uint64_t          s_start_us   = 0;
static esp_timer_handle_t s_timer     = NULL;

// ---------------------------------------------------------------------------

static const char* state_str(eTaskState s) {
    switch (s) {
        case eRunning:   return "RUN";
        case eReady:     return "RDY";
        case eBlocked:   return "BLK";
        case eSuspended: return "SUS";
        case eDeleted:   return "DEL";
        default:         return "UNK";
    }
}

// Devuelve el ultimo runtime conocido para este handle y actualiza la tabla.
// Si es la primera vez que vemos el handle, lo registra y devuelve current
// (delta = 0 en la primera muestra, que es correcto).
static unsigned long track_get_and_update(TaskHandle_t h, const char* name,
                                          unsigned long current_rt) {
    for (uint32_t i = 0; i < s_track_n; i++) {
        if (s_track[i].handle == h) {
            unsigned long prev = s_track[i].last_runtime;
            s_track[i].last_runtime = current_rt;
            return prev;
        }
    }
    // Tarea nueva
    if (s_track_n < MAX_TASKS) {
        s_track[s_track_n].handle       = h;
        s_track[s_track_n].last_runtime = current_rt;
        s_track[s_track_n].valid        = 1;
        strncpy(s_track[s_track_n].name, name, configMAX_TASK_NAME_LEN - 1);
        s_track_n++;
    }
    return current_rt;  // delta = 0, primera vez
}

// ---------------------------------------------------------------------------

void task_tracer_dump_now(void) {
    unsigned long total_rt = 0;
    UBaseType_t n = uxTaskGetSystemState(s_status, MAX_TASKS, &total_rt);
    if (n == 0) return;

    unsigned long delta_total = (total_rt >= s_last_total)
                                ? (total_rt - s_last_total)
                                : total_rt;   // wraparound improbable en U32 a 1kHz
    s_last_total = total_rt;

    uint64_t ts_ms = (esp_timer_get_time() - s_start_us) / 1000ULL;

    for (UBaseType_t i = 0; i < n; i++) {
        TaskStatus_t* t = &s_status[i];

        unsigned long prev_rt = track_get_and_update(t->xHandle,
                                                     t->pcTaskName,
                                                     t->ulRunTimeCounter);
        unsigned long delta_rt = (t->ulRunTimeCounter >= prev_rt)
                                 ? (t->ulRunTimeCounter - prev_rt)
                                 : 0;

        // cpu_pct_x10 = (delta_rt / delta_total) * 1000
        // Evitamos float usando aritmetica entera.
        uint32_t cpu_x10 = (delta_total > 0)
                           ? (uint32_t)((uint64_t)delta_rt * 1000ULL / delta_total)
                           : 0u;

        uint32_t hwm = (uint32_t)uxTaskGetStackHighWaterMark(t->xHandle);

        char core_s[12];
        if ((int)t->xCoreID == tskNO_AFFINITY)
            strncpy(core_s, "ANY", sizeof(core_s));
        else
            snprintf(core_s, sizeof(core_s), "%d", (int)t->xCoreID);

        // Linea parseble por Python. Separar con \n limpio.
        printf("TRACE,%llu,%s,%s,%lu,%lu,%s\n",
               (unsigned long long)ts_ms,
               t->pcTaskName,
               state_str(t->eCurrentState),
               (unsigned long)cpu_x10,   // cpu%*10, ej 125 = 12.5%
               (unsigned long)hwm,
               core_s);
    }
    printf("TRACE_END,%llu\n", (unsigned long long)ts_ms);
}

static void timer_cb(void* arg) {
    task_tracer_dump_now();
}

void task_tracer_start(uint32_t interval_ms) {
    if (s_timer) return;

    s_start_us   = esp_timer_get_time();
    s_last_total = 0;
    s_track_n    = 0;

    const esp_timer_create_args_t args = {
        .callback = timer_cb,
        .arg      = NULL,
        .name     = "task_tracer",
    };
    if (esp_timer_create(&args, &s_timer) != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo crear el timer");
        return;
    }
    esp_timer_start_periodic(s_timer, (uint64_t)interval_ms * 1000ULL);
    ESP_LOGI(TAG, "Task tracer iniciado: muestreo cada %lu ms", (unsigned long)interval_ms);
    ESP_LOGI(TAG, "Captura el log con: idf.py -p COMX monitor | tee captura.log");
    ESP_LOGI(TAG, "Analiza con:        python tools/trace_analyze.py captura.log");
}

void task_tracer_stop(void) {
    if (!s_timer) return;
    esp_timer_stop(s_timer);
    esp_timer_delete(s_timer);
    s_timer = NULL;
    ESP_LOGI(TAG, "Task tracer detenido");
}

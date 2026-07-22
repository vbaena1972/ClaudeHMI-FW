// mem_diag.c
#include "mem_diag.h"
#include "esp_system.h"
#include "esp_timer.h"
#include <stdio.h>
#include <string.h>

static const char* MEMTAG = "MEM";

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static const char* task_state_str(eTaskState s) {
    switch (s) {
        case eRunning:   return "RUN";
        case eReady:     return "RDY";
        case eBlocked:   return "BLK";
        case eSuspended: return "SUS";
        case eDeleted:   return "DEL";
        default:         return "???";
    }
}

// ---------------------------------------------------------------------------
// Heap reports
// ---------------------------------------------------------------------------

void mem_diag_report(const char* tag) {
    size_t free_int      = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    size_t free_psram    = heap_caps_get_free_size(MALLOC_CAP_SPIRAM   | MALLOC_CAP_8BIT);
    size_t free_all      = free_int + free_psram;
    size_t largest_int   = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    size_t largest_psram = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM   | MALLOC_CAP_8BIT);

    ESP_LOGI(MEMTAG, "[%s] HEAP total=%u  INT=%u (largest=%u)  PSRAM=%u (largest=%u)",
             tag, (unsigned)free_all,
             (unsigned)free_int, (unsigned)largest_int,
             (unsigned)free_psram, (unsigned)largest_psram);
}

void mem_diag_report_assert_min(size_t min_internal_free,
                                size_t min_internal_largest,
                                const char* tag) {
    size_t free_int    = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    size_t largest_int = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    ESP_LOGI(MEMTAG, "[%s] INT=%u (largest=%u)", tag,
             (unsigned)free_int, (unsigned)largest_int);

    if (free_int < min_internal_free || largest_int < min_internal_largest) {
        ESP_LOGE(MEMTAG,
                 "[%s] BELOW THRESHOLD: free_int=%u (min=%u), largest_int=%u (min=%u)",
                 tag, (unsigned)free_int, (unsigned)min_internal_free,
                 (unsigned)largest_int, (unsigned)min_internal_largest);
    }
}

// Heap con minimo historico y % de fragmentacion.
// frag% = 0 → heap contiguo ideal; frag% alto → bloques dispersos, malloc grande fallara pronto.
void mem_diag_report_heap_full(const char* tag) {
    size_t free_int      = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    size_t free_psram    = heap_caps_get_free_size(MALLOC_CAP_SPIRAM   | MALLOC_CAP_8BIT);
    size_t min_int       = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    size_t min_psram     = heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM   | MALLOC_CAP_8BIT);
    size_t largest_int   = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    size_t largest_psram = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM   | MALLOC_CAP_8BIT);

    uint32_t frag_int   = (free_int   > 0) ? (100u - (unsigned)(largest_int   * 100u / free_int))   : 0u;
    uint32_t frag_psram = (free_psram > 0) ? (100u - (unsigned)(largest_psram * 100u / free_psram)) : 0u;

    ESP_LOGI(MEMTAG, "[%s] === HEAP ===", tag);
    ESP_LOGI(MEMTAG, "  INT  : free=%6u  min_ever=%6u  largest=%6u  frag=%2u%%",
             (unsigned)free_int, (unsigned)min_int, (unsigned)largest_int, (unsigned)frag_int);
    ESP_LOGI(MEMTAG, "  PSRAM: free=%6u  min_ever=%6u  largest=%6u  frag=%2u%%",
             (unsigned)free_psram, (unsigned)min_psram, (unsigned)largest_psram, (unsigned)frag_psram);
}

// ---------------------------------------------------------------------------
// Task report  (requiere CONFIG_FREERTOS_USE_TRACE_FACILITY=y y
//               CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y)
//
// CPU%: relativo al tiempo de reloj (wall-clock).  En dual-core el maximo
//       teorico es 200% (100% por core).  Un idle task al ~50% significa
//       que ese core esta libre ~la mitad del tiempo.
// Stack watermark: palabras libres minimas vistas.  Si llega a 0 → desborde.
// ---------------------------------------------------------------------------
void mem_diag_report_tasks(void) {
    volatile UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();
    TaskStatus_t *tasks = malloc(uxArraySize * sizeof(TaskStatus_t));
    if (!tasks) {
        ESP_LOGW(MEMTAG, "No RAM para listar tareas");
        return;
    }

    unsigned long ulTotalRunTime = 0;
    uxArraySize = uxTaskGetSystemState(tasks, uxArraySize, &ulTotalRunTime);

    ESP_LOGI(MEMTAG, "=== TASKS  (wall_time=%lu) ===", ulTotalRunTime);
    ESP_LOGI(MEMTAG, "%-16s %-3s %4s %5s %6s %4s",
             "Name", "St", "Prio", "CPU%", "StkHW", "Core");
    ESP_LOGI(MEMTAG, "---------------- --- ---- ----- ------ ----");

    for (UBaseType_t i = 0; i < uxArraySize; i++) {
        TaskStatus_t *t = &tasks[i];
        UBaseType_t watermark = uxTaskGetStackHighWaterMark(t->xHandle);

        uint32_t cpu_pct = (ulTotalRunTime > 0)
            ? (uint32_t)((t->ulRunTimeCounter * 100ULL) / ulTotalRunTime)
            : 0u;

        char core_str[12];
        if ((int)t->xCoreID == tskNO_AFFINITY) {
            strncpy(core_str, "ANY", sizeof(core_str));
        } else {
            snprintf(core_str, sizeof(core_str), "%d", (int)t->xCoreID);
        }

        ESP_LOGI(MEMTAG, "%-16s %-3s %4lu %4lu%% %6lu %4s",
                 t->pcTaskName,
                 task_state_str(t->eCurrentState),
                 (unsigned long)t->uxCurrentPriority,
                 (unsigned long)cpu_pct,
                 (unsigned long)watermark,
                 core_str);
    }
    free(tasks);
}

// ---------------------------------------------------------------------------
// Reporte combinado
// ---------------------------------------------------------------------------
void mem_diag_report_full(const char* tag) {
    ESP_LOGI(MEMTAG, "");
    ESP_LOGI(MEMTAG, "======= DIAG [%s] =======", tag);
    mem_diag_report_heap_full(tag);
    mem_diag_report_tasks();
    ESP_LOGI(MEMTAG, "=========================");
    ESP_LOGI(MEMTAG, "");
}

// ---------------------------------------------------------------------------
// Timer periodico (sin tarea extra, sin stack dedicado)
// ---------------------------------------------------------------------------
static esp_timer_handle_t s_diag_timer = NULL;

static void diag_timer_cb(void* arg) {
    mem_diag_report_full("PERIODIC");
}

void mem_diag_start_periodic(uint32_t interval_ms) {
    if (s_diag_timer) return;
    const esp_timer_create_args_t args = {
        .callback = diag_timer_cb,
        .arg      = NULL,
        .name     = "mem_diag",
    };
    if (esp_timer_create(&args, &s_diag_timer) != ESP_OK) {
        ESP_LOGE(MEMTAG, "No se pudo crear el timer de diagnostico");
        return;
    }
    esp_timer_start_periodic(s_diag_timer, (uint64_t)interval_ms * 1000ULL);
    ESP_LOGI(MEMTAG, "Diagnostico periodico cada %lu ms", (unsigned long)interval_ms);
}

void mem_diag_stop_periodic(void) {
    if (!s_diag_timer) return;
    esp_timer_stop(s_diag_timer);
    esp_timer_delete(s_diag_timer);
    s_diag_timer = NULL;
}

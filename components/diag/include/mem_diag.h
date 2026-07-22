// mem_diag.h
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Heap ---

// Reporte rapido: free actual de INT y PSRAM
void mem_diag_report(const char* tag);

// Reporte completo: free + min_ever + largest + frag%
void mem_diag_report_heap_full(const char* tag);

// Falla en log si la heap interna baja de los umbrales dados
void mem_diag_report_assert_min(size_t min_internal_free,
                                size_t min_internal_largest,
                                const char* tag);

// --- Tareas ---

// Tabla de tareas: nombre, estado, prioridad, CPU%, stack watermark, core
// Requiere: CONFIG_FREERTOS_USE_TRACE_FACILITY=y
//           CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y
void mem_diag_report_tasks(void);

// --- Combinado ---

// Llama a mem_diag_report_heap_full + mem_diag_report_tasks
void mem_diag_report_full(const char* tag);

// --- Timer periodico (sin tarea extra) ---

// Inicia un esp_timer que llama a mem_diag_report_full cada interval_ms ms
void mem_diag_start_periodic(uint32_t interval_ms);
void mem_diag_stop_periodic(void);

#ifdef __cplusplus
}
#endif

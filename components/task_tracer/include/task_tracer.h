// task_tracer.h
// Muestreador periodico de tareas FreeRTOS.
// Emite lineas "TRACE,..." por el puerto serie que el script
// tools/trace_analyze.py convierte en graficas.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Inicia el muestreo cada interval_ms milisegundos.
// Los datos se emiten por printf (mismo puerto que ESP_LOGI).
void task_tracer_start(uint32_t interval_ms);

// Detiene el muestreo.
void task_tracer_stop(void);

// Vuelca una sola muestra inmediatamente (sin necesidad de que el timer corra).
void task_tracer_dump_now(void);

#ifdef __cplusplus
}
#endif

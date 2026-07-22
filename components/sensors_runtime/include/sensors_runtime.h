#ifndef SENSORS_RUNTIME_H
#define SENSORS_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "storage.h"  // Para AppConfig (cfg->sensors.*)

/**
 * Muestra básica de sensores en unidades internas (SI):
 *  - Presión: kPa
 *  - Flujo:   L/min
 */
typedef struct
{
    int64_t ts_ms;        ///< timestamp en milisegundos (esp_timer_get_time()/1000)
    float   pressure_kpa; ///< presión en kPa
    float   flow_lpm;     ///< flujo en L/min
} sensor_sample_t;

/**
 * Inicializa el runtime de sensores:
 *  - Crea el buffer circular
 *  - Inicializa mutex
 *  - Arranca la tarea dummy de generación de muestras (opcional, para pruebas)
 *
 * IMPORTANTE:
 *  - cfg puede ser NULL; en ese caso se usan defaults internos para la tarea dummy.
 *  - Cuando luego tengas el driver real, podrás llamar a sensors_runtime_push_sample()
 *    desde tu propia tarea en vez de usar la dummy.
 */
bool sensors_runtime_init(const AppConfig *cfg);

/**
 * Actualiza la configuración (por si cambias calibración, etc).
 * Por ahora la guardamos solo por si luego quieres usar cal.* aquí.
 */
void sensors_runtime_update_config(const AppConfig *cfg);

/**
 * Inserta una muestra en el buffer circular.
 * Se puede llamar:
 *  - desde la tarea dummy interna
 *  - o desde tu driver real de sensores.
 */
void sensors_runtime_push_sample(const sensor_sample_t *s);

/**
 * Obtiene la última muestra disponible (la más reciente).
 * Devuelve true si hay datos válidos, false si el buffer está vacío.
 */
bool sensors_runtime_get_last(sensor_sample_t *out);

/**
 * Calcula min y max de presión y flujo en una ventana de tiempo [now - window_ms, now].
 * - window_ms: ventana en milisegundos (ej: 60000 para 1 minuto).
 * - min_out / max_out pueden ser NULL si no te interesa alguno.
 * Devuelve true si encontró muestras en la ventana, false si no hay datos.
 */
bool sensors_runtime_get_min_max(int64_t window_ms,
                                 sensor_sample_t *min_out,
                                 sensor_sample_t *max_out);

/**
 * Extrae una serie de muestras dentro de la ventana de tiempo (para gráficas).
 * - window_ms: ventana hacia atrás en ms desde "ahora".
 * - out: buffer donde se copiarán las muestras (en orden cronológico ascendente).
 * - max: capacidad del buffer out.
 * - out_count: cantidad real copiada.
 * Devuelve true si hay al menos 1 muestra copiada.
 */
bool sensors_runtime_get_series(int64_t window_ms,
                                sensor_sample_t *out,
                                size_t max,
                                size_t *out_count);

                                /**
 * Extrae Min, Max y Promedio (Avg) de la ventana de tiempo especificada.
 * Ideal para construir el Payload JSON de AWS IoT.
 */
bool sensors_runtime_get_aws_telemetry(int64_t window_ms,
                                       float *p_min, float *p_max, float *p_avg,
                                       float *f_min, float *f_max, float *f_avg);
#endif // SENSORS_RUNTIME_H

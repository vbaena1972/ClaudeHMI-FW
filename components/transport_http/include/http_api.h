#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Inicializa el módulo HTTP (mutex, colas internas, etc.). No arranca el server.
 * Debe llamarse una sola vez en el arranque del sistema (p.ej. desde app_main()).
 */
esp_err_t http_api_init(void);

/**
 * Arranca el servidor HTTP de forma idempotente (si ya está, no hace nada).
 * Recomendado llamarlo DESPUÉS de obtener IP (IP_EVENT_STA_GOT_IP).
 */
esp_err_t http_api_start(void);

/**
 * Detiene el servidor HTTP si está corriendo (opcional).
 */
void http_api_stop(void);

#ifdef __cplusplus
}
#endif

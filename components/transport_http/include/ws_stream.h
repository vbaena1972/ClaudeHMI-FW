#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "esp_http_server.h"

/**
 * Registra el endpoint WebSocket en /api/v1/stream sobre un httpd ya iniciado.
 * Si CONFIG_HTTPD_WS_SUPPORT está en 'n', devuelve ESP_OK pero no hace nada.
 */
esp_err_t ws_stream_start(httpd_handle_t server);

/**
 * Envía (broadcast) un texto JSON a todos los clientes WS conectados.
 * Si no hay soporte WS o no hay clientes, retorna ESP_OK igualmente.
 */
esp_err_t ws_stream_broadcast(const char *json_text);

/**
 * Helper para emitir un evento estándar cuando cambia la config.
 */
void ws_emit_cfg_changed(int rev);

#ifdef __cplusplus
}
#endif

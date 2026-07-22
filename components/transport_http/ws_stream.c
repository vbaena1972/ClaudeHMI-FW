#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "ws_stream";

#if CONFIG_HTTPD_WS_SUPPORT

static httpd_handle_t s_server = NULL;

static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET)
        return ESP_OK; // handshake
    httpd_ws_frame_t ws_pkt = {
        .type = HTTPD_WS_TYPE_TEXT,
        .final = true,
        .payload = NULL,
        .len = 0};
    // leer y descartar (eco opcional)
    if (httpd_ws_recv_frame(req, &ws_pkt, 0) == ESP_OK && ws_pkt.len)
    {
        char *buf = malloc(ws_pkt.len + 1);
        if (!buf)
            return ESP_ERR_NO_MEM;
        ws_pkt.payload = (uint8_t *)buf;
        if (httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len) == ESP_OK)
        {
            buf[ws_pkt.len] = 0;
            // ESP_LOGD(TAG, "WS RX: %s", buf);
        }
        free(buf);
    }
    return ESP_OK;
}

esp_err_t ws_stream_start(httpd_handle_t server)
{
    s_server = server;
    httpd_uri_t u = {
        .uri = "/api/v1/stream",
        .method = HTTP_GET,
        .handler = ws_handler,
        .is_websocket = true};
    return httpd_register_uri_handler(server, &u);
}

esp_err_t ws_stream_broadcast(const char *json_text)
{
    if (!s_server || !json_text)
        return ESP_FAIL;
    httpd_ws_frame_t ws_pkt = {
        .type = HTTPD_WS_TYPE_TEXT,
        .final = true,
        .payload = (uint8_t *)json_text,
        .len = strlen(json_text)};
    // Intento simple: probar fds 0..63 (suficiente para pruebas)
    for (int fd = 0; fd < 64; ++fd)
    {
        if (httpd_ws_get_fd_info(s_server, fd) == HTTPD_WS_CLIENT_WEBSOCKET)
        {
            httpd_ws_send_frame_async(s_server, fd, &ws_pkt);
        }
    }
    return ESP_OK;
}

void ws_emit_cfg_changed(int rev)
{
    cJSON *o = cJSON_CreateObject();
    cJSON_AddStringToObject(o, "type", "cfg.changed");
    cJSON_AddNumberToObject(o, "rev", rev);
    char *txt = cJSON_PrintUnformatted(o);
    cJSON_Delete(o);
    ws_stream_broadcast(txt);
    free(txt);
}

#else // CONFIG_HTTPD_WS_SUPPORT off

esp_err_t ws_stream_start(httpd_handle_t server)
{
    (void)server;
    return ESP_OK;
}
esp_err_t ws_stream_broadcast(const char *json_text)
{
    (void)json_text;
    return ESP_OK;
}
void ws_emit_cfg_changed(int rev) { (void)rev; }

#endif

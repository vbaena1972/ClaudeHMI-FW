#include <string.h>
#include <inttypes.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_app_desc.h" // esp_app_get_description()
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "cJSON.h"
#include "http_api.h"
#include "state_pub.h"
#include "mem_diag.h"

static esp_err_t state_get_handler(httpd_req_t *req);

static const char *TAG = "http_api";
static httpd_handle_t s_httpd = NULL;
static SemaphoreHandle_t s_lock;

// ----------- Config por defecto (puedes mover a Kconfig si quieres) -----------
#ifndef HTTPD_413_PAYLOAD_TOO_LARGE
#define HTTPD_413_PAYLOAD_TOO_LARGE HTTPD_413_CONTENT_TOO_LARGE
#endif

#ifndef HTTP_API_PORT
#define HTTP_API_PORT (80)
#endif

#ifndef HTTP_API_CTRL_PORT
#define HTTP_API_CTRL_PORT (32768)
#endif

#ifndef HTTP_API_STACK_SIZE
#define HTTP_API_STACK_SIZE (8192) // sube a 12288 si serializas JSON grandes
#endif

#ifndef HTTP_API_MAX_SOCKETS
#define HTTP_API_MAX_SOCKETS (4)
#endif

#ifndef HTTP_API_RECV_BUF
#define HTTP_API_RECV_BUF (2048) // leer cuerpo POST en trozos
#endif
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------
// Utilidades
// ------------------------------------------------------------------------------------
// static void httpd_log_cfg(const httpd_config_t *cfg, esp_err_t err, const char *prefix)
// {
//     if (err == ESP_OK)
//     {
//         ESP_LOGI(TAG, "%s on :%d (ctrl:%d, stack=%u, sockets=%d)",
//                  prefix, (int)cfg->server_port, (int)cfg->ctrl_port,
//                  (unsigned)cfg->stack_size, (int)cfg->max_open_sockets);
//     }
//     else
//     {
//         ESP_LOGE(TAG, "%s failed: %s (port:%d ctrl:%d stack:%u sockets:%d)",
//                  prefix, esp_err_to_name(err), (int)cfg->server_port,
//                  (int)cfg->ctrl_port, (unsigned)cfg->stack_size,
//                  (int)cfg->max_open_sockets);
//     }
// }

static esp_err_t send_json_chunk_begin(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    return httpd_resp_sendstr_chunk(req, "{");
}

static esp_err_t send_json_chunk_kv(httpd_req_t *req, const char *k, const char *v, bool last, bool quote_val)
{
    // envia ,"k":"v"  o  ,"k":v
    esp_err_t err;
    err = httpd_resp_sendstr_chunk(req, "\"");
    if (err != ESP_OK)
        return err;
    err = httpd_resp_sendstr_chunk(req, k);
    if (err != ESP_OK)
        return err;
    err = httpd_resp_sendstr_chunk(req, "\":");
    if (err != ESP_OK)
        return err;

    if (quote_val)
    {
        err = httpd_resp_sendstr_chunk(req, "\"");
        if (err != ESP_OK)
            return err;
        err = httpd_resp_sendstr_chunk(req, v ? v : "");
        if (err != ESP_OK)
            return err;
        err = httpd_resp_sendstr_chunk(req, "\"");
        if (err != ESP_OK)
            return err;
    }
    else
    {
        err = httpd_resp_sendstr_chunk(req, v ? v : "null");
        if (err != ESP_OK)
            return err;
    }

    if (!last)
    {
        err = httpd_resp_sendstr_chunk(req, ",");
    }
    return err;
}

static esp_err_t send_json_chunk_end(httpd_req_t *req)
{
    esp_err_t err = httpd_resp_sendstr_chunk(req, "}");
    if (err != ESP_OK)
        return err;
    return httpd_resp_sendstr_chunk(req, NULL); // fin de chunked
}

// Lee el cuerpo de una petición (POST/PUT) en trozos al buffer dado,
// devolviendo el total leído. No añade '\0' automáticamente si body binario.
// static int recv_body_into_buf(httpd_req_t *req, char *buf, size_t buf_len)
// {
//     int remaining = req->content_len;
//     int total = 0;

//     while (remaining > 0)
//     {
//         size_t to_read = remaining > (int)buf_len ? buf_len : remaining;
//         int r = httpd_req_recv(req, buf, to_read);
//         if (r <= 0)
//         {
//             if (r == HTTPD_SOCK_ERR_TIMEOUT)
//             {
//                 continue;
//             }
//             return r; // error
//         }
//         remaining -= r;
//         total += r;
//     }
//     return total;
// }

// ------------------------------------------------------------------------------------
// /api/v1/info  (GET)
// ------------------------------------------------------------------------------------
static esp_err_t info_get_handler(httpd_req_t *req)
{
    const esp_app_desc_t *ad = esp_app_get_description();

    esp_err_t err = send_json_chunk_begin(req);
    if (err != ESP_OK)
        return err;

    // {"ok":true,
    err = httpd_resp_sendstr_chunk(req, "\"ok\":true,");
    if (err != ESP_OK)
        return err;

    // "project": "...", "fw": "...", "idf": "..."
    err = send_json_chunk_kv(req, "project", ad ? ad->project_name : "unknown", false, true);
    if (err)
        return err;
    err = send_json_chunk_kv(req, "fw", ad ? ad->version : "unknown", false, true);
    if (err)
        return err;
    err = send_json_chunk_kv(req, "idf", ad ? ad->idf_ver : "unknown", true, true);
    if (err)
        return err;

    return send_json_chunk_end(req);
}

// ------------------------------------------------------------------------------------
// /api/v1/config (GET)  -> devuelve la config actual como JSON
// Nota: si ya tienes una función que vuelca el JSON (ej. appcfg_dump_json),
// úsala. Aquí dejo un stub seguro.
// ------------------------------------------------------------------------------------
static esp_err_t cfg_get_handler(httpd_req_t *req)
{
    // Si ya tienes JSON listo en RAM (p. ej., char* json;):
    // httpd_resp_set_type(req, "application/json");
    // return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);

    // Stub mínimo:
    esp_err_t err = send_json_chunk_begin(req);
    if (err != ESP_OK)
        return err;
    err = httpd_resp_sendstr_chunk(req, "\"ok\":true,");
    if (err != ESP_OK)
        return err;
    err = httpd_resp_sendstr_chunk(req, "\"config\":{}");
    if (err != ESP_OK)
        return err;
    return send_json_chunk_end(req);
}

// ------------------------------------------------------------------------------------
// /api/v1/config (PUT)  -> aplica un patch JSON
// Espera algo como: {"base_rev":1,"patch":{...}}
// Llama a tu appcfg_patch() si la tienes. Aquí parsea y valida tamaño.
// ------------------------------------------------------------------------------------
static esp_err_t cfg_put_handler(httpd_req_t *req)
{
    if (req->content_len <= 0 || req->content_len > (128 * 1024))
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid content length");
        return ESP_OK;
    }

    // Reservamos un buffer moderado y parseamos en streaming a cJSON
    char *buf = (char *)malloc(HTTP_API_RECV_BUF + 1);
    if (!buf)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");
        return ESP_OK;
    }

    cJSON *root = NULL;
    cJSON *patch = NULL;
    int remaining = req->content_len;

    // Acumulamos en un cJSON Parser con string temporal (para no copiarlo entero en RAM)
    // Estrategia simple: si el contenido cabe en <= 32 KB lo leemos todo; si no, rechaza.
    const int max_body = (32 * 1024);
    char *full = NULL;
    if (req->content_len > max_body)
    {
        free(buf);
        //        httpd_resp_send_err(req, HTTPD_413_PAYLOAD_TOO_LARGE, "body too large");
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "body too large");

        return ESP_OK;
    }

    full = (char *)malloc(req->content_len + 1);
    if (!full)
    {
        free(buf);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");
        return ESP_OK;
    }

    int copied = 0;
    while (remaining > 0)
    {
        int to_read = remaining > HTTP_API_RECV_BUF ? HTTP_API_RECV_BUF : remaining;
        int r = httpd_req_recv(req, buf, to_read);
        if (r <= 0)
        {
            if (r == HTTPD_SOCK_ERR_TIMEOUT)
                continue;
            free(buf);
            free(full);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "recv failed");
            return ESP_OK;
        }
        memcpy(full + copied, buf, r);
        copied += r;
        remaining -= r;
    }
    full[copied] = '\0';
    free(buf);

    root = cJSON_ParseWithLength(full, copied);
    if (!root)
    {
        free(full);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid json");
        return ESP_OK;
    }

    // Extrae "patch"
    patch = cJSON_GetObjectItem(root, "patch");
    if (!patch || !cJSON_IsObject(patch))
    {
        cJSON_Delete(root); // OJO: borra todo, incluido 'patch'
        free(full);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing patch");
        return ESP_OK;
    }

    // Si tienes una función que aplica el patch sobre tu storage:
    // esp_err_t perr = appcfg_patch(patch);  // IMPORTANTE: NO hagas cJSON_Delete(patch) si la función se apropia
    // if (perr != ESP_OK) { ... }

    // Stub: responde ok sin aplicar nada
    {
        esp_err_t err = send_json_chunk_begin(req);
        if (err == ESP_OK)
        {
            httpd_resp_sendstr_chunk(req, "\"ok\":true");
            send_json_chunk_end(req);
        }
    }

    // Como NO hemos transferido propiedad de 'root/patch' a nadie, ahora sí borramos:
    cJSON_Delete(root);
    free(full);
    return ESP_OK;
}

// ------------------------------------------------------------------------------------
// Registro de URIs
// ------------------------------------------------------------------------------------
static void register_uris(httpd_handle_t server)
{
    const httpd_uri_t uri_info_get = {
        .uri = "/api/v1/info",
        .method = HTTP_GET,
        .handler = info_get_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server, &uri_info_get);

    httpd_uri_t uri_state_get = {
        .uri = "/api/v1/state",
        .method = HTTP_GET,
        .handler = state_get_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server, &uri_state_get);

    const httpd_uri_t uri_cfg_get = {
        .uri = "/api/v1/config",
        .method = HTTP_GET,
        .handler = cfg_get_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server, &uri_cfg_get);

    const httpd_uri_t uri_cfg_put = {
        .uri = "/api/v1/config",
        .method = HTTP_PUT,
        .handler = cfg_put_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server, &uri_cfg_put);
}

static esp_err_t state_get_handler(httpd_req_t *req)
{
    char json[256];
    state_build_json(json, sizeof json);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

// ------------------------------------------------------------------------------------
// Hook opcional a eventos de red: arranca al tener IP (solo una vez)
// ------------------------------------------------------------------------------------
// static void on_got_ip(void *arg, esp_event_base_t base, int32_t id, void *data)
// {
//     (void)arg;
//     (void)base;
//     (void)id;
//     (void)data;
//     // Arranque idempotente
//     esp_err_t err = http_api_start();
//     if (err != ESP_OK)
//     {
//         // Si falla por recursos, no lo martilles: deja log y ya
//         ESP_LOGE(TAG, "start after GOT_IP failed: %s", esp_err_to_name(err));
//     }
// }

esp_err_t http_api_init(void)
{
    if (!s_lock)
        s_lock = xSemaphoreCreateMutex();
    return s_lock ? ESP_OK : ESP_ERR_NO_MEM;
}

static esp_err_t http_api__start_locked(void)
{
    if (s_httpd)
    {
        ESP_LOGI(TAG, "httpd already running");
        return ESP_OK;
    }
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.lru_purge_enable = true; // purga handlers viejos si hace falta
    cfg.stack_size = 6144;       // ajusta si tu build lo requiere

    esp_err_t err = httpd_start(&s_httpd, &cfg);
    if (err != ESP_OK)
    {
        s_httpd = NULL;
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        return err;
    }
    register_uris(s_httpd);
    ESP_LOGI(TAG, "httpd started");
    return ESP_OK;
}

// ------------------------------------------------------------------------------------
// Arranque/parada idempotente del servidor
// ------------------------------------------------------------------------------------
esp_err_t http_api_start(void)
{
    if (!s_lock)
        return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(s_lock, portMAX_DELAY);

    mem_diag_report("HTTP:before_start");
    esp_err_t r = http_api__start_locked();
    mem_diag_report("HTTP:after_start");

    xSemaphoreGive(s_lock);
    return r;
}

void http_api_stop(void)
{
    if (!s_lock)
        return;
    xSemaphoreTake(s_lock, portMAX_DELAY);

    mem_diag_report("HTTP:before_stop");
    if (s_httpd)
    {
        httpd_stop(s_httpd);
        s_httpd = NULL;
        ESP_LOGI(TAG, "httpd stopped");
    }
    mem_diag_report("HTTP:after_stop");

    xSemaphoreGive(s_lock);
}

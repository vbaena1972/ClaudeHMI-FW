#include "metrics_store.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

#define MET_NS  "metrics"
#define MET_KEY "m"

static app_metrics_t s_cache;      /* última copia conocida (para la UI) */
static bool s_cache_ok = false;

esp_err_t appmetrics_load(app_metrics_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));

    nvs_handle_t h;
    esp_err_t r = nvs_open(MET_NS, NVS_READONLY, &h);
    if (r == ESP_OK) {
        size_t len = sizeof(*out);
        r = nvs_get_blob(h, MET_KEY, out, &len);
        nvs_close(h);
        if (r != ESP_OK || len != sizeof(*out))
            memset(out, 0, sizeof(*out));   /* primera vez / blob de otra versión */
    }
    out->date[sizeof(out->date) - 1] = '\0';
    s_cache = *out;
    s_cache_ok = true;
    return ESP_OK;
}

esp_err_t appmetrics_store(const app_metrics_t *in)
{
    if (!in) return ESP_ERR_INVALID_ARG;

    nvs_handle_t h;
    esp_err_t r = nvs_open(MET_NS, NVS_READWRITE, &h);
    if (r != ESP_OK) return r;
    r = nvs_set_blob(h, MET_KEY, in, sizeof(*in));
    if (r == ESP_OK) r = nvs_commit(h);
    nvs_close(h);

    if (r == ESP_OK) { s_cache = *in; s_cache_ok = true; }
    return r;
}

uint32_t appmetrics_service_min(void)
{
    return s_cache_ok ? s_cache.service_min : 0;
}

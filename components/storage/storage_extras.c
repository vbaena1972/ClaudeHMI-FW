#include "storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"
#include "esp_timer.h"
#include <string.h>
#include <stdio.h>

// Double-slot write (atomic-ish): cfg_a/cfg_b + active key
#define NVS_NS "cfg"
#define NVS_KEY_ACTIVE "active_slot"
#define NVS_KEY_A "appcfg_a"
#define NVS_KEY_B "appcfg_b"

static const char* slot_key(char slot) { return slot=='A' ? NVS_KEY_A : NVS_KEY_B; }
static char other_slot(char slot) { return slot=='A' ? 'B' : 'A'; }

// ---------- Helpers ----------
static void set_str(char *dst, size_t cap, const char *src) {
    if (!dst || cap == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    size_t n = strnlen(src, cap-1);
    memcpy(dst, src, n);
    dst[n] = '\0';
}

// Simple FNV-1a 32-bit
static uint32_t fnv1a32(const uint8_t *data, size_t len) {
    uint32_t hash = 2166136261u;
    for (size_t i=0;i<len;i++) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

// Serializa AppConfig a JSON sin espacios (para etag)
static char* appcfg_to_minified_json(const AppConfig *c) {
    extern cJSON* json_from_cfg(const AppConfig *c); // defined in storage.c
    cJSON *root = json_from_cfg(c);
    if (!root) return NULL;
    char *txt = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return txt;
}

// RFC 7396 JSON Merge Patch (OBJ <- PATCH). Devuelve 1 ok, 0 error.
static int json_merge_patch_obj(cJSON *obj, const cJSON *patch) {
    if (!cJSON_IsObject(obj) || !cJSON_IsObject(patch)) return 0;
    for (const cJSON *it = patch->child; it; it = it->next) {
        const char *k = it->string;
        cJSON *dst = cJSON_GetObjectItemCaseSensitive(obj, k);
        if (cJSON_IsNull(it)) {
            // delete key
            if (dst) cJSON_DeleteItemFromObjectCaseSensitive(obj, k);
            continue;
        }
        if (cJSON_IsObject(it)) {
            if (!dst || !cJSON_IsObject(dst)) {
                // replace
                cJSON_ReplaceItemInObjectCaseSensitive(obj, k, cJSON_Duplicate(it, 1));
            } else {
                if (!json_merge_patch_obj(dst, it)) return 0;
            }
        } else {
            cJSON_ReplaceItemInObjectCaseSensitive(obj, k, cJSON_Duplicate(it, 1));
        }
    }
    return 1;
}

// Valida rangos básicos y consistencias
bool appcfg_validate(const AppConfig *c) {
    if (!c) return false;
    // Units
    if (!(strcmp(c->sensors.pressure_unit,"bar")==0 ||
          strcmp(c->sensors.pressure_unit,"kpa")==0 ||
          strcmp(c->sensors.pressure_unit,"psi")==0)) return false;
    if (!(strcmp(c->sensors.flow_unit,"lpm")==0 ||
          strcmp(c->sensors.flow_unit,"slpm")==0 ||
          strcmp(c->sensors.flow_unit,"nlpm")==0 ||
          strcmp(c->sensors.flow_unit,"sccm")==0)) return false;
    // Alarms
    if (c->sensors.alarm_limits.pressure_min < 0) return false;
    if (c->sensors.alarm_limits.pressure_max <= c->sensors.alarm_limits.pressure_min) return false;
    if (c->sensors.alarm_limits.flow_delta_threshold < 0) return false;
    if (c->sensors.alarm_limits.flow_delta_window_ms < 50 || c->sensors.alarm_limits.flow_delta_window_ms > 60000) return false;
    // Network sanity
    if (strlen(c->wifi.ip_mode) == 0) return false;
    // OK
    return true;
}

void appcfg_calc_etag(AppConfig *c) {
    if (!c) return;
    char *min = appcfg_to_minified_json(c);
    if (!min) { set_str(c->etag, sizeof(c->etag), "00000000"); return; }
    uint32_t h = fnv1a32((const uint8_t*)min, strlen(min));
    free(min);
    char hex[9]; snprintf(hex, sizeof(hex), "%08x", (unsigned)h);
    set_str(c->etag, sizeof(c->etag), hex);
}

static esp_err_t read_active(char *slot) {
    nvs_handle_t h;
    esp_err_t r = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (r != ESP_OK) return r;
    size_t len = 1; r = nvs_get_blob(h, NVS_KEY_ACTIVE, slot, &len);
    nvs_close(h);
    if (r == ESP_ERR_NVS_NOT_FOUND) { *slot = 'A'; return ESP_OK; }
    return r;
}

static esp_err_t write_active(char slot) {
    nvs_handle_t h; esp_err_t r = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (r != ESP_OK) return r;
    r = nvs_set_blob(h, NVS_KEY_ACTIVE, &slot, 1);
    if (r == ESP_OK) r = nvs_commit(h);
    nvs_close(h);
    return r;
}

static esp_err_t read_slot_json(char slot, char **out, size_t *outlen) {
    *out = NULL; *outlen = 0;
    nvs_handle_t h; esp_err_t r = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (r != ESP_OK) return r;
    size_t len=0; r = nvs_get_blob(h, slot_key(slot), NULL, &len);
    if (r != ESP_OK) { nvs_close(h); return r; }
    char *buf = (char*)malloc(len+1); if (!buf) { nvs_close(h); return ESP_ERR_NO_MEM; }
    r = nvs_get_blob(h, slot_key(slot), buf, &len);
    nvs_close(h);
    if (r != ESP_OK) { free(buf); return r; }
    buf[len] = '\0'; *out = buf; *outlen = len;
    return ESP_OK;
}

static esp_err_t write_slot_json(char slot, const char *json) {
    nvs_handle_t h; esp_err_t r = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (r != ESP_OK) return r;
    r = nvs_set_blob(h, slot_key(slot), json, strlen(json));
    if (r == ESP_OK) r = nvs_commit(h);
    nvs_close(h);
    return r;
}

// Replace full config with base_rev check
cfg_result_t appcfg_set(int base_rev, const char *cfg_json_full, const char *updated_by) {
    if (!cfg_json_full) return CFG_BAD_REQUEST;
    AppConfig current; if (appcfg_load(&current) != ESP_OK) return CFG_PERSIST_ERR;
    if (base_rev >= 0 && base_rev != current.rev) return CFG_CONFLICT;

    cJSON *root = cJSON_Parse(cfg_json_full);
    if (!root) return CFG_BAD_REQUEST;
    AppConfig next; // start from defaults then apply full
    // We will reuse storage.c cfg_from_json via parsing -> to struct.
    extern void cfg_from_json(AppConfig *c, const cJSON *root); // defined in storage.c
    cfg_from_json(&next, root);
    cJSON_Delete(root);

    if (!appcfg_validate(&next)) return CFG_VALIDATION_ERR;

    next.rev = current.rev + 1;
    set_str(next.updated_by, sizeof(next.updated_by), updated_by ? updated_by : "http");
    next.updated_ts_ms = (uint64_t)(esp_timer_get_time() / 1000ULL);
    appcfg_calc_etag(&next);

    char active='A'; read_active(&active);
    char inactive = other_slot(active);

    char *txt = appcfg_to_minified_json(&next);
    if (!txt) return CFG_PERSIST_ERR;

    if (write_slot_json(inactive, txt) != ESP_OK) { free(txt); return CFG_PERSIST_ERR; }
    if (write_active(inactive) != ESP_OK) { free(txt); return CFG_PERSIST_ERR; }
    free(txt);
    // also keep compatibility with old key if someone calls appcfg_save
    appcfg_save(&next);
    return CFG_OK;
}

// Merge-patch with base_rev
cfg_result_t appcfg_patch(int base_rev, const char *patch_json, const char *updated_by) {
    if (!patch_json) return CFG_BAD_REQUEST;

    AppConfig current; if (appcfg_load(&current) != ESP_OK) return CFG_PERSIST_ERR;
    if (base_rev >= 0 && base_rev != current.rev) return CFG_CONFLICT;

    // current -> JSON
    char *cur_txt = appcfg_to_minified_json(&current);
    if (!cur_txt) return CFG_PERSIST_ERR;
    cJSON *cur = cJSON_Parse(cur_txt); free(cur_txt);
    if (!cur) return CFG_PERSIST_ERR;

    cJSON *patch = cJSON_Parse(patch_json);
    if (!patch) { cJSON_Delete(cur); return CFG_BAD_REQUEST; }

    if (!json_merge_patch_obj(cur, patch)) { cJSON_Delete(cur); cJSON_Delete(patch); return CFG_BAD_REQUEST; }
    cJSON_Delete(patch);

    // JSON -> struct
    AppConfig next;
    extern void cfg_from_json(AppConfig *c, const cJSON *root); // storage.c
    cfg_from_json(&next, cur);
    cJSON_Delete(cur);

    if (!appcfg_validate(&next)) return CFG_VALIDATION_ERR;

    next.rev = current.rev + 1;
    set_str(next.updated_by, sizeof(next.updated_by), updated_by ? updated_by : "http");
    next.updated_ts_ms = (uint64_t)(esp_timer_get_time() / 1000ULL);
    appcfg_calc_etag(&next);

    char active='A'; read_active(&active);
    char inactive = other_slot(active);
    char *txt = appcfg_to_minified_json(&next);
    if (!txt) return CFG_PERSIST_ERR;
    if (write_slot_json(inactive, txt) != ESP_OK) { free(txt); return CFG_PERSIST_ERR; }
    if (write_active(inactive) != ESP_OK) { free(txt); return CFG_PERSIST_ERR; }
    free(txt);
    // compatibility write
    appcfg_save(&next);
    return CFG_OK;
}

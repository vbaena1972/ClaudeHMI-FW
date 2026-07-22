#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include "cJSON.h" // <-- importa cJSON para los prototipos

#ifdef __cplusplus
extern "C"
{
#endif

    // ---------------- AppConfig (JSON) ----------------
    typedef enum
    {
        APP_BT_SEC_JW = 0,     // Just Works
        APP_BT_SEC_PASSKEY = 1 // Passkey (PIN de 6 dígitos)
    } app_bt_sec_mode_t;

    typedef enum
    {
        MUY_BAJO = 0,
        BAJO,
        BAJO_MEDIO,
        MEDIO,
        MEDIO_ALTO,
        ALTO,
        MUY_ALTO,
        MAXIMO,
    } app_bt_tx_power_t;

    typedef struct
    {
        // GENERAL
        struct
        {
            char info_text[32]; // 20-25 chars
            char datetime[32];  // ISO8601 o epoch string
            char timezone[32];  // "America/Bogota"
            char lang[3];       // "es" | "en"

            // Identidad fija del Hardware (Solo lectura desde el exterior)
            char model[16];
            char serial[24];
            char hw_version[16];

            // Versión del Software (Fijada por compilación)
            char fw_version[16];

            // Datos de despliegue comercial (Modificables por SD)
            char partner[32];
            char client[64];
            struct
            {
                bool tone_warn;
                bool tone_alert;
                int warn_timeout_s;
                int alert_timeout_s;
            } alarm;
            // Usuarios (simple): aquí solo 1 admin para no alargar
            struct
            {
                char user[16];
                char pass[16];
            } admin;
        } general;

        // SENSORES
        struct
        {
            char pressure_unit[8]; // "bar"|"kpa"|"psi"
            char flow_unit[8];     // "lpm"|"slpm"|"nlpm"|"sccm"
            char gas_type[16];     // "o2"|"air_med"|...
            char color_code[8];    // "iso"|"nfpa"
            struct
            {
                float pressure_offset, pressure_scale;
                float flow_offset, flow_scale;
            } cal;
            struct
            {
                float pressure_min, pressure_max;
                float flow_delta_threshold;
                int flow_delta_window_ms;
            } alarm_limits;
        } sensors;

        // WIFI
        struct
        {
            bool enabled;
            char ssid[33];
            char password[33];
            char ip_mode[8]; // "dhcp"|"static"
            char ip[16], mask[16], gw[16], dns1[16], dns2[16];
        } wifi;

        // ETHERNET
        struct
        {
            bool enabled;
            char ip_mode[8];
            char ip[16], mask[16], gw[16], dns1[16], dns2[16];
            char hostname[33];
        } eth;

        // BLUETOOTH (placeholder mesh+classic)
        struct
        {

            bool enabled;
            bool advertise;             // control explícito de advertising
            app_bt_tx_power_t tx_power; // 0..7 (o el rango que uses)
            struct
            {
                bool enabled;
                bool provisioned;
                uint8_t ttl;
                bool relay;
                char net_key[64];
                char app_key[64];
                char dev_uuid[40];
                uint16_t unicast_addr;
            } mesh;
            struct
            {
                char name[24];
                char pin[8];
                app_bt_sec_mode_t sec_mode; // JW o PASSKEY
            } legacy;
        } bt;

        // CLOUD (MQTT)
        struct
        {
            bool enabled;
            char type[8]; // "mqtt"|"http"
            char broker_url[128];
            char topic_base[64];
            char ota_url[128];
            int qos;
            int keepalive;
        } cloud;

        // ---- Metadatos de config ----
        char schema[16];        // "config@2"
        int rev;                // autoincremental al aplicar cambios
        char etag[16];          // hash corto (e.g., FNV-1a/xxhash en hex)
        char updated_by[12];    // "device" | "http" | "ble" | "aws"
        uint64_t updated_ts_ms; // epoch msF
        // versionado de schema
        int ver;
    } AppConfig;

    typedef enum
    {
        CFG_OK = 0,
        CFG_CONFLICT,
        CFG_BAD_REQUEST,
        CFG_VALIDATION_ERR,
        CFG_PERSIST_ERR
    } cfg_result_t;

    // Carga/guarda JSON completo en NVS
    void appcfg_defaults(AppConfig *c);
    esp_err_t appcfg_migrate(AppConfig *io);

    // Prototipos para serializar/deserializar AppConfig a/desde JSON (cJSON)
    cJSON *json_from_cfg(const AppConfig *c); // devuelve un cJSON* que DEBES liberar con cJSON_Delete()
    void cfg_from_json(AppConfig *c, const cJSON *root);

    esp_err_t appcfg_load(AppConfig *out);
    esp_err_t appcfg_save(const AppConfig *in); // ya existe

    // Nuevas:
    cfg_result_t appcfg_set(int base_rev, const char *cfg_json_full, const char *updated_by);
    cfg_result_t appcfg_patch(int base_rev, const char *patch_json, const char *updated_by);

    void appcfg_calc_etag(AppConfig *c);      // hash del JSON sin espacios
    bool appcfg_validate(const AppConfig *c); // rangos/consistencias

    // --- Snapshot en RAM interna (para uso seguro desde LVGL) ---
    esp_err_t appcfg_cache_reload(void);        // carga desde NVS a snapshot interno
    esp_err_t appcfg_cache_get(AppConfig *out); // copia el snapshot actual a 'out'
    AppConfig *appcfg_cache_peek(void);

#ifdef __cplusplus
}
#endif

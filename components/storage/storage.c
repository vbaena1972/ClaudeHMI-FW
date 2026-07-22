#include "storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"

#define NVS_NS "cfg"
#define NVS_KEY "appcfg"

static void set_str(char *dst, size_t cap, const char *src)
{
    if (!dst || cap == 0)
        return;
    if (!src)
    {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, cap - 1);
    dst[cap - 1] = '\0';
}

static void getstr(char *dst, size_t cap, const cJSON *obj, const char *key)
{
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsString(v) && v->valuestring)
        set_str(dst, cap, v->valuestring);
}

void appcfg_defaults(AppConfig *c)
{
    memset(c, 0, sizeof(*c));
    // c->ver = 1;
    set_str(c->schema, sizeof(c->schema), "config@2");
    c->rev = 1;
    set_str(c->etag, sizeof(c->etag), "00000000");
    set_str(c->updated_by, sizeof(c->updated_by), "device");
    c->updated_ts_ms = 0;
    c->ver = 2;
    // general
    set_str(c->general.info_text, sizeof(c->general.info_text), "Axira Flow monitor");
    set_str(c->general.datetime, sizeof(c->general.datetime), "");
    set_str(c->general.timezone, sizeof(c->general.timezone), "America/Bogota");
    set_str(c->general.lang, sizeof(c->general.lang), "es");
    // Identidad fija del Hardware (Solo lectura desde el exterior)
    set_str(c->general.model, sizeof(c->general.model), "FPM-200");
    set_str(c->general.serial, sizeof(c->general.serial), "fpm-0001");
    set_str(c->general.hw_version, sizeof(c->general.hw_version), "1.1");

    // Versión del Software (Fijada por compilación)
    set_str(c->general.fw_version, sizeof(c->general.fw_version), "1.0.0");

    // Datos de despliegue comercial (Modificables por SD)
    set_str(c->general.partner, sizeof(c->general.partner), "Vexel_Partner");
    set_str(c->general.client, sizeof(c->general.client), "lab_medellin");

    c->general.alarm.tone_warn = true;
    c->general.alarm.tone_alert = true;
    c->general.alarm.warn_timeout_s = 5;
    c->general.alarm.alert_timeout_s = 10;
    set_str(c->general.admin.user, sizeof(c->general.admin.user), "admin");
    set_str(c->general.admin.pass, sizeof(c->general.admin.pass), "1234");

    // sensors
    set_str(c->sensors.pressure_unit, sizeof(c->sensors.pressure_unit), "psi");
    set_str(c->sensors.flow_unit, sizeof(c->sensors.flow_unit), "lpm");
    set_str(c->sensors.gas_type, sizeof(c->sensors.gas_type), "o2");
    set_str(c->sensors.color_code, sizeof(c->sensors.color_code), "iso");
    c->sensors.cal.pressure_offset = 0.f;
    c->sensors.cal.pressure_scale = 1.f;
    c->sensors.cal.flow_offset = 0.f;
    c->sensors.cal.flow_scale = 1.f;
    c->sensors.alarm_limits.pressure_min = 350.f;
    c->sensors.alarm_limits.pressure_max = 1000.f;
    c->sensors.alarm_limits.flow_delta_threshold = 15.f;
    c->sensors.alarm_limits.flow_delta_window_ms = 2000;

    // wifi
    c->wifi.enabled = false;
    set_str(c->wifi.ssid, sizeof(c->wifi.ssid), "SUSSY");
    set_str(c->wifi.password, sizeof(c->wifi.password), "25111943");
    set_str(c->wifi.ip_mode, sizeof(c->wifi.ip_mode), "dhcp");
    set_str(c->wifi.ip, sizeof(c->wifi.ip), "192.168.1.248");
    set_str(c->wifi.mask, sizeof(c->wifi.mask), "255.255.255.0");
    set_str(c->wifi.gw, sizeof(c->wifi.gw), "192.168.1.1");
    set_str(c->wifi.dns1, sizeof(c->wifi.dns1), "192.168.1.1");
    set_str(c->wifi.dns2, sizeof(c->wifi.dns2), "192.168.1.1");

    // eth
    c->eth.enabled = false;
    set_str(c->eth.hostname, sizeof(c->eth.hostname), "axira_host");
    set_str(c->eth.ip_mode, sizeof(c->eth.ip_mode), "dhcp");
    set_str(c->eth.ip, sizeof(c->eth.ip), "192.168.1.248");
    set_str(c->eth.mask, sizeof(c->eth.mask), "255.255.255.0");
    set_str(c->eth.gw, sizeof(c->eth.gw), "192.168.1.1");
    set_str(c->eth.dns1, sizeof(c->eth.dns1), "192.168.1.1");
    set_str(c->eth.dns2, sizeof(c->eth.dns2), "192.168.1.1");

    // bt
    c->bt.enabled = false;
    c->bt.advertise = false;
    c->bt.tx_power = MEDIO;

    c->bt.mesh.enabled = false;
    c->bt.mesh.provisioned = false;
    c->bt.mesh.ttl = 5;
    c->bt.mesh.relay = false;
    set_str(c->bt.mesh.net_key, sizeof(c->bt.mesh.net_key), "00112233445566778899AABBCCDDEEFF");
    set_str(c->bt.mesh.app_key, sizeof(c->bt.mesh.app_key), "FFEEDDCCBBAA99887766554433221100");
    set_str(c->bt.mesh.dev_uuid, sizeof(c->bt.mesh.dev_uuid), "123e4567-e89b-12d3-a456-426614174000");
    c->bt.mesh.unicast_addr = 0x0002;

    set_str(c->bt.legacy.name, sizeof(c->bt.legacy.name), "AXIRAsystem");
    set_str(c->bt.legacy.pin, sizeof(c->bt.legacy.pin), "0000");
    c->bt.legacy.sec_mode = APP_BT_SEC_JW;

    // cloud
    c->cloud.enabled = false;
    set_str(c->cloud.type, sizeof(c->cloud.type), "mqtt");
    // Tu endpoint real de AWS IoT Core (¡Este está perfecto, déjalo así!)
    set_str(c->cloud.broker_url, sizeof(c->cloud.broker_url), "mqtts://a13o0atpzl606i-ats.iot.us-east-1.amazonaws.com:8883");
    // Raíz del tópico MQTT (El código en transport_mqtt.c le agregará automáticamente "/telemetry" al final)
    set_str(c->cloud.topic_base, sizeof(c->cloud.topic_base), "vexel");
    c->cloud.qos = 1;        // QOS 1 es ideal para asegurar que AWS reciba la telemetría
    c->cloud.keepalive = 60; // 60 segundos es el estándar más estable para AWS IoT (30s es un poco agresivo y puede causar reconexiones)
}

cJSON *json_from_cfg(const AppConfig *c)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "ver", c->ver);

    cJSON_AddStringToObject(root, "schema", c->schema);
    cJSON_AddNumberToObject(root, "rev", c->rev);
    cJSON_AddStringToObject(root, "etag", c->etag);
    cJSON_AddStringToObject(root, "updated_by", c->updated_by);
    cJSON_AddNumberToObject(root, "updated_ts_ms", (double)c->updated_ts_ms);

    // general
    cJSON *gen = cJSON_AddObjectToObject(root, "general");
    cJSON_AddStringToObject(gen, "info_text", c->general.info_text);
    cJSON_AddStringToObject(gen, "datetime", c->general.datetime);
    cJSON_AddStringToObject(gen, "timezone", c->general.timezone);
    cJSON_AddStringToObject(gen, "lang", c->general.lang);
    // Identidad fija del Hardware (Solo lectura desde el exterior)
    cJSON_AddStringToObject(gen, "model", c->general.model);
    cJSON_AddStringToObject(gen, "serial", c->general.serial);
    cJSON_AddStringToObject(gen, "hw_version", c->general.hw_version);
    // Versión del Software (Fijada por compilación)
    cJSON_AddStringToObject(gen, "fw_version", c->general.fw_version);
    // Datos de despliegue comercial (Modificables por SD)
    cJSON_AddStringToObject(gen, "partner", c->general.partner);
    cJSON_AddStringToObject(gen, "client", c->general.client);

    cJSON *al = cJSON_AddObjectToObject(gen, "alarm");
    cJSON_AddBoolToObject(al, "tone_warn", c->general.alarm.tone_warn);
    cJSON_AddBoolToObject(al, "tone_alert", c->general.alarm.tone_alert);
    cJSON_AddNumberToObject(al, "warn_timeout_s", c->general.alarm.warn_timeout_s);
    cJSON_AddNumberToObject(al, "alert_timeout_s", c->general.alarm.alert_timeout_s);
    cJSON *adm = cJSON_AddObjectToObject(gen, "admin");
    cJSON_AddStringToObject(adm, "user", c->general.admin.user);
    cJSON_AddStringToObject(adm, "pass", c->general.admin.pass);

    cJSON *sns = cJSON_AddObjectToObject(root, "sensors");
    cJSON_AddStringToObject(sns, "pressure_unit", c->sensors.pressure_unit);
    cJSON_AddStringToObject(sns, "flow_unit", c->sensors.flow_unit);
    cJSON_AddStringToObject(sns, "gas_type", c->sensors.gas_type);
    cJSON_AddStringToObject(sns, "color_code", c->sensors.color_code);
    cJSON *cal = cJSON_AddObjectToObject(sns, "cal");
    cJSON_AddNumberToObject(cal, "pressure_offset", c->sensors.cal.pressure_offset);
    cJSON_AddNumberToObject(cal, "pressure_scale", c->sensors.cal.pressure_scale);
    cJSON_AddNumberToObject(cal, "flow_offset", c->sensors.cal.flow_offset);
    cJSON_AddNumberToObject(cal, "flow_scale", c->sensors.cal.flow_scale);
    cJSON *al2 = cJSON_AddObjectToObject(sns, "alarm_limits");
    cJSON_AddNumberToObject(al2, "pressure_min", c->sensors.alarm_limits.pressure_min);
    cJSON_AddNumberToObject(al2, "pressure_max", c->sensors.alarm_limits.pressure_max);
    cJSON_AddNumberToObject(al2, "flow_delta_threshold", c->sensors.alarm_limits.flow_delta_threshold);
    cJSON_AddNumberToObject(al2, "flow_delta_window_ms", c->sensors.alarm_limits.flow_delta_window_ms);

    cJSON *wf = cJSON_AddObjectToObject(root, "wifi");
    cJSON_AddBoolToObject(wf, "enabled", c->wifi.enabled);
    cJSON_AddStringToObject(wf, "ssid", c->wifi.ssid);
    cJSON_AddStringToObject(wf, "password", c->wifi.password);
    cJSON_AddStringToObject(wf, "ip_mode", c->wifi.ip_mode);
    cJSON_AddStringToObject(wf, "ip", c->wifi.ip);
    cJSON_AddStringToObject(wf, "mask", c->wifi.mask);
    cJSON_AddStringToObject(wf, "gw", c->wifi.gw);
    cJSON_AddStringToObject(wf, "dns1", c->wifi.dns1);
    cJSON_AddStringToObject(wf, "dns2", c->wifi.dns2);

    cJSON *eth = cJSON_AddObjectToObject(root, "eth");
    cJSON_AddBoolToObject(eth, "enabled", c->eth.enabled);
    cJSON_AddStringToObject(eth, "hostname", c->eth.hostname);
    cJSON_AddStringToObject(eth, "ip_mode", c->eth.ip_mode);
    cJSON_AddStringToObject(eth, "ip", c->eth.ip);
    cJSON_AddStringToObject(eth, "mask", c->eth.mask);
    cJSON_AddStringToObject(eth, "gw", c->eth.gw);
    cJSON_AddStringToObject(eth, "dns1", c->eth.dns1);
    cJSON_AddStringToObject(eth, "dns2", c->eth.dns2);

    cJSON *bt = cJSON_AddObjectToObject(root, "bt");
    cJSON_AddBoolToObject(bt, "enabled", c->bt.enabled);
    cJSON_AddBoolToObject(bt, "advertise", c->bt.advertise);
    cJSON_AddNumberToObject(bt, "tx_power", c->bt.tx_power);

    cJSON *lg = cJSON_AddObjectToObject(bt, "legacy");
    cJSON_AddStringToObject(lg, "name", c->bt.legacy.name);
    cJSON_AddStringToObject(lg, "pin", c->bt.legacy.pin);
    cJSON_AddNumberToObject(bt, "sec_mode", c->bt.legacy.sec_mode);

    cJSON *mesh = cJSON_AddObjectToObject(bt, "mesh");
    cJSON_AddBoolToObject(mesh, "enabled", c->bt.mesh.enabled);
    cJSON_AddBoolToObject(mesh, "provisioned", c->bt.mesh.provisioned);
    cJSON_AddNumberToObject(mesh, "ttl", c->bt.mesh.ttl);
    cJSON_AddBoolToObject(mesh, "relay", c->bt.mesh.relay);
    cJSON_AddStringToObject(mesh, "net_key", c->bt.mesh.net_key);
    cJSON_AddStringToObject(mesh, "app_key", c->bt.mesh.app_key);
    cJSON_AddStringToObject(mesh, "dev_uuid", c->bt.mesh.dev_uuid);
    cJSON_AddNumberToObject(mesh, "unicast_addr", c->bt.mesh.unicast_addr);

    cJSON *cl = cJSON_AddObjectToObject(root, "cloud");
    cJSON_AddBoolToObject(cl, "enabled", c->cloud.enabled);
    cJSON_AddStringToObject(cl, "type", c->cloud.type);
    cJSON_AddStringToObject(cl, "broker_url", c->cloud.broker_url);
    cJSON_AddStringToObject(cl, "topic_base", c->cloud.topic_base);
    cJSON_AddNumberToObject(cl, "qos", c->cloud.qos);
    cJSON_AddNumberToObject(cl, "keepalive", c->cloud.keepalive);
    cJSON_AddStringToObject(cl, "ota_url", c->cloud.ota_url);

    return root;
}

void cfg_from_json(AppConfig *c, const cJSON *root)
{
    if (!root)
        return;

    ESP_LOGI("STORAGE", "Iniciando parseo profundo de AppConfig.json...");

    // Obtener versión raíz
    const cJSON *ver = cJSON_GetObjectItemCaseSensitive(root, "ver");
    if (cJSON_IsNumber(ver))
        c->ver = ver->valueint;

    // --- RAMA GENERAL ---
    cJSON *general = cJSON_GetObjectItemCaseSensitive(root, "general");
    if (general)
    {
        ESP_LOGI("STORAGE", "Parseando rama: 'general'");
        // Campos modificables permitidos
        getstr(c->general.info_text, sizeof(c->general.info_text), general, "info_text");
        getstr(c->general.datetime, sizeof(c->general.datetime), general, "datetime");
        getstr(c->general.timezone, sizeof(c->general.timezone), general, "timezone");
        getstr(c->general.lang, sizeof(c->general.lang), general, "lang");
        getstr(c->general.partner, sizeof(c->general.partner), general, "partner");
        getstr(c->general.client, sizeof(c->general.client), general, "client");

        cJSON *alarm = cJSON_GetObjectItemCaseSensitive(general, "alarm");
        if (alarm)
        {
            const cJSON *t_warn = cJSON_GetObjectItemCaseSensitive(alarm, "tone_warn");
            const cJSON *t_alert = cJSON_GetObjectItemCaseSensitive(alarm, "tone_alert");
            const cJSON *w_to = cJSON_GetObjectItemCaseSensitive(alarm, "warn_timeout_s");
            const cJSON *a_to = cJSON_GetObjectItemCaseSensitive(alarm, "alert_timeout_s");

            if (cJSON_IsBool(t_warn))
                c->general.alarm.tone_warn = cJSON_IsTrue(t_warn);
            if (cJSON_IsBool(t_alert))
                c->general.alarm.tone_alert = cJSON_IsTrue(t_alert);
            if (cJSON_IsNumber(w_to))
                c->general.alarm.warn_timeout_s = w_to->valueint;
            if (cJSON_IsNumber(a_to))
                c->general.alarm.alert_timeout_s = a_to->valueint;
        }

        cJSON *adm = cJSON_GetObjectItemCaseSensitive(general, "admin");
        if (adm)
        {
            getstr(c->general.admin.user, sizeof(c->general.admin.user), adm, "user");
            getstr(c->general.admin.pass, sizeof(c->general.admin.pass), adm, "pass");
        }
    }

    // --- RAMA SENSORS ---
    cJSON *sensors = cJSON_GetObjectItemCaseSensitive(root, "sensors");
    if (sensors)
    {
        ESP_LOGI("STORAGE", "Parseando rama: 'sensors' (y calibraciones)");
        getstr(c->sensors.pressure_unit, sizeof(c->sensors.pressure_unit), sensors, "pressure_unit");
        getstr(c->sensors.flow_unit, sizeof(c->sensors.flow_unit), sensors, "flow_unit");
        getstr(c->sensors.gas_type, sizeof(c->sensors.gas_type), sensors, "gas_type");
        getstr(c->sensors.color_code, sizeof(c->sensors.color_code), sensors, "color_code");

        cJSON *cal = cJSON_GetObjectItemCaseSensitive(sensors, "cal");
        if (cal)
        {
            const cJSON *po = cJSON_GetObjectItemCaseSensitive(cal, "pressure_offset");
            const cJSON *ps = cJSON_GetObjectItemCaseSensitive(cal, "pressure_scale");
            const cJSON *fo = cJSON_GetObjectItemCaseSensitive(cal, "flow_offset");
            const cJSON *fs = cJSON_GetObjectItemCaseSensitive(cal, "flow_scale");

            if (cJSON_IsNumber(po))
                c->sensors.cal.pressure_offset = po->valuedouble;
            if (cJSON_IsNumber(ps))
                c->sensors.cal.pressure_scale = ps->valuedouble;
            if (cJSON_IsNumber(fo))
                c->sensors.cal.flow_offset = fo->valuedouble;
            if (cJSON_IsNumber(fs))
                c->sensors.cal.flow_scale = fs->valuedouble;
        }

        cJSON *limits = cJSON_GetObjectItemCaseSensitive(sensors, "alarm_limits");
        if (limits)
        {
            const cJSON *p_min = cJSON_GetObjectItemCaseSensitive(limits, "pressure_min");
            const cJSON *p_max = cJSON_GetObjectItemCaseSensitive(limits, "pressure_max");
            const cJSON *f_dt = cJSON_GetObjectItemCaseSensitive(limits, "flow_delta_threshold");
            const cJSON *f_dw = cJSON_GetObjectItemCaseSensitive(limits, "flow_delta_window_ms");

            if (cJSON_IsNumber(p_min))
                c->sensors.alarm_limits.pressure_min = p_min->valuedouble;
            if (cJSON_IsNumber(p_max))
                c->sensors.alarm_limits.pressure_max = p_max->valuedouble;
            if (cJSON_IsNumber(f_dt))
                c->sensors.alarm_limits.flow_delta_threshold = f_dt->valuedouble;
            if (cJSON_IsNumber(f_dw))
                c->sensors.alarm_limits.flow_delta_window_ms = f_dw->valueint;
        }
    }

    // --- RAMA WIFI ---
    cJSON *wifi = cJSON_GetObjectItemCaseSensitive(root, "wifi");
    if (wifi)
    {
        ESP_LOGI("STORAGE", "Parseando rama: 'wifi'");
        const cJSON *en = cJSON_GetObjectItemCaseSensitive(wifi, "enabled");
        if (cJSON_IsBool(en))
            c->wifi.enabled = cJSON_IsTrue(en);

        getstr(c->wifi.ssid, sizeof(c->wifi.ssid), wifi, "ssid");
        getstr(c->wifi.password, sizeof(c->wifi.password), wifi, "password");
        getstr(c->wifi.ip_mode, sizeof(c->wifi.ip_mode), wifi, "ip_mode");
        getstr(c->wifi.ip, sizeof(c->wifi.ip), wifi, "ip");
        getstr(c->wifi.mask, sizeof(c->wifi.mask), wifi, "mask");
        getstr(c->wifi.gw, sizeof(c->wifi.gw), wifi, "gw");
        getstr(c->wifi.dns1, sizeof(c->wifi.dns1), wifi, "dns1");
        getstr(c->wifi.dns2, sizeof(c->wifi.dns2), wifi, "dns2");
    }

    // --- RAMA CLOUD ---
    cJSON *cloud = cJSON_GetObjectItemCaseSensitive(root, "cloud");
    if (cloud)
    {
        ESP_LOGI("STORAGE", "Parseando rama: 'cloud' (AWS MQTT)");
        const cJSON *en = cJSON_GetObjectItemCaseSensitive(cloud, "enabled");
        if (cJSON_IsBool(en))
            c->cloud.enabled = cJSON_IsTrue(en);

        getstr(c->cloud.type, sizeof(c->cloud.type), cloud, "type");
        getstr(c->cloud.broker_url, sizeof(c->cloud.broker_url), cloud, "broker_url");
        getstr(c->cloud.topic_base, sizeof(c->cloud.topic_base), cloud, "topic_base");
        getstr(c->cloud.ota_url, sizeof(c->cloud.ota_url), cloud, "ota_url");

        const cJSON *qos = cJSON_GetObjectItemCaseSensitive(cloud, "qos");
        if (cJSON_IsNumber(qos))
            c->cloud.qos = qos->valueint;

        const cJSON *keepalive = cJSON_GetObjectItemCaseSensitive(cloud, "keepalive");
        if (cJSON_IsNumber(keepalive))
            c->cloud.keepalive = keepalive->valueint;
    }

    // --- RAMA ETH ---
    cJSON *eth = cJSON_GetObjectItemCaseSensitive(root, "eth");
    if (eth)
    {
        ESP_LOGI("STORAGE", "Parseando rama: 'eth'");
        const cJSON *en = cJSON_GetObjectItemCaseSensitive(eth, "enabled");
        if (cJSON_IsBool(en))
            c->eth.enabled = cJSON_IsTrue(en);
        getstr(c->eth.hostname, sizeof(c->eth.hostname), eth, "hostname");
        getstr(c->eth.ip_mode, sizeof(c->eth.ip_mode), eth, "ip_mode");
        getstr(c->eth.ip, sizeof(c->eth.ip), eth, "ip");
        getstr(c->eth.mask, sizeof(c->eth.mask), eth, "mask");
        getstr(c->eth.gw, sizeof(c->eth.gw), eth, "gw");
        getstr(c->eth.dns1, sizeof(c->eth.dns1), eth, "dns1");
        getstr(c->eth.dns2, sizeof(c->eth.dns2), eth, "dns2");
    }

    // --- RAMA BLUETOOTH ---
    cJSON *bt = cJSON_GetObjectItemCaseSensitive(root, "bt");
    if (bt)
    {
        ESP_LOGI("STORAGE", "Parseando rama: 'bt' (Mesh y Legacy)");
        const cJSON *en = cJSON_GetObjectItemCaseSensitive(bt, "enabled");
        const cJSON *adv = cJSON_GetObjectItemCaseSensitive(bt, "advertise");
        const cJSON *tx = cJSON_GetObjectItemCaseSensitive(bt, "tx_power");
        const cJSON *sec = cJSON_GetObjectItemCaseSensitive(bt, "sec_mode");

        if (cJSON_IsBool(en))
            c->bt.enabled = cJSON_IsTrue(en);
        if (cJSON_IsBool(adv))
            c->bt.advertise = cJSON_IsTrue(adv);
        if (cJSON_IsNumber(tx))
            c->bt.tx_power = tx->valueint;
        if (cJSON_IsNumber(sec))
            c->bt.legacy.sec_mode = sec->valueint;

        cJSON *lg = cJSON_GetObjectItemCaseSensitive(bt, "legacy");
        if (lg)
        {
            getstr(c->bt.legacy.name, sizeof(c->bt.legacy.name), lg, "name");
            getstr(c->bt.legacy.pin, sizeof(c->bt.legacy.pin), lg, "pin");
        }

        cJSON *mesh = cJSON_GetObjectItemCaseSensitive(bt, "mesh");
        if (mesh)
        {
            const cJSON *m_en = cJSON_GetObjectItemCaseSensitive(mesh, "enabled");
            const cJSON *m_prov = cJSON_GetObjectItemCaseSensitive(mesh, "provisioned");
            const cJSON *m_ttl = cJSON_GetObjectItemCaseSensitive(mesh, "ttl");
            const cJSON *m_relay = cJSON_GetObjectItemCaseSensitive(mesh, "relay");
            const cJSON *m_uni = cJSON_GetObjectItemCaseSensitive(mesh, "unicast_addr");

            if (cJSON_IsBool(m_en))
                c->bt.mesh.enabled = cJSON_IsTrue(m_en);
            if (cJSON_IsBool(m_prov))
                c->bt.mesh.provisioned = cJSON_IsTrue(m_prov);
            if (cJSON_IsBool(m_relay))
                c->bt.mesh.relay = cJSON_IsTrue(m_relay);
            if (cJSON_IsNumber(m_ttl))
                c->bt.mesh.ttl = m_ttl->valueint;
            if (cJSON_IsNumber(m_uni))
                c->bt.mesh.unicast_addr = m_uni->valueint;

            getstr(c->bt.mesh.net_key, sizeof(c->bt.mesh.net_key), mesh, "net_key");
            getstr(c->bt.mesh.app_key, sizeof(c->bt.mesh.app_key), mesh, "app_key");
            getstr(c->bt.mesh.dev_uuid, sizeof(c->bt.mesh.dev_uuid), mesh, "dev_uuid");
        }
    }
    ESP_LOGI("STORAGE", "Parseo profundo de AppConfig.json completado.");
}

esp_err_t appcfg_load(AppConfig *out)
{
    if (!out)
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t h;
    esp_err_t r = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (r != ESP_OK)
    {
        appcfg_defaults(out);
        return ESP_OK;
    }

    size_t len = 0;
    r = nvs_get_blob(h, NVS_KEY, NULL, &len);
    if (r == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_close(h);
        appcfg_defaults(out);
        return ESP_OK;
    }
    if (r != ESP_OK || len == 0)
    {
        nvs_close(h);
        appcfg_defaults(out);
        return ESP_OK;
    }

    char *buf = (char *)malloc(len + 1);
    if (!buf)
    {
        nvs_close(h);
        return ESP_ERR_NO_MEM;
    }
    r = nvs_get_blob(h, NVS_KEY, buf, &len);
    nvs_close(h);
    if (r != ESP_OK)
    {
        free(buf);
        appcfg_defaults(out);
        return ESP_OK;
    }

    buf[len] = '\0';
    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root)
    {
        appcfg_defaults(out);
        return ESP_OK;
    }

    cfg_from_json(out, root);
    cJSON_Delete(root);

    // migraciones si subimos de versión
    appcfg_migrate(out);
    return ESP_OK;
}

esp_err_t appcfg_save(const AppConfig *in)
{
    if (!in)
        return ESP_ERR_INVALID_ARG;

    cJSON *root = json_from_cfg(in);
    if (!root)
        return ESP_ERR_NO_MEM;
    char *txt = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!txt)
        return ESP_ERR_NO_MEM;

    nvs_handle_t h;
    esp_err_t r = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (r != ESP_OK)
    {
        free(txt);
        return r;
    }

    r = nvs_set_blob(h, NVS_KEY, txt, strlen(txt));
    if (r == ESP_OK)
        r = nvs_commit(h);
    nvs_close(h);
    free(txt);
    return r;
}

esp_err_t appcfg_migrate(AppConfig *io)
{
    // ejemplo de migración entre versiones (ahora solo ver==1)
    if (io->ver < 1)
    {
        // ... ajustes si venías de 0 (no lo usamos)
        io->ver = 1;
    }
    return ESP_OK;
}

// Snapshot interno en DRAM (estático)
static AppConfig s_cfg_snapshot;
static bool s_cfg_snapshot_inited = false;

esp_err_t appcfg_cache_reload(void)
{
    AppConfig tmp;
    appcfg_defaults(&tmp); // por si falla NVS
    esp_err_t r = appcfg_load(&tmp);
    // copiamos siempre (si falla, quedan defaults)
    memcpy(&s_cfg_snapshot, &tmp, sizeof(AppConfig));
    s_cfg_snapshot_inited = true;
    return r;
}

esp_err_t appcfg_cache_get(AppConfig *out)
{
    if (!out)
        return ESP_ERR_INVALID_ARG;
    if (!s_cfg_snapshot_inited)
    {
        // primera vez: defaults
        appcfg_defaults(&s_cfg_snapshot);
        s_cfg_snapshot_inited = true;
    }
    memcpy(out, &s_cfg_snapshot, sizeof(AppConfig));
    return ESP_OK;
}

AppConfig *appcfg_cache_peek(void)
{
    if (!s_cfg_snapshot_inited)
    {
        appcfg_defaults(&s_cfg_snapshot);
        s_cfg_snapshot_inited = true;
    }
    return &s_cfg_snapshot; // ⚠ No modificar desde fuera
}

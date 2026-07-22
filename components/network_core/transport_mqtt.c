#include "esp_log.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "cJSON.h"
#include <string.h>

#include "storage.h"
#include "cert_store.h"
#include "sensors_runtime.h"
#include "state_pub.h"

static const char *TAG = "aws_mqtt";

static esp_mqtt_client_handle_t s_client = NULL;
static bool s_is_connected = false;

// === AGREGA ESTAS TRES LÍNEAS AQUÍ ===
static char *s_safe_ca = NULL;
static char *s_safe_cert = NULL;
static char *s_safe_key = NULL;

// === NUEVO: Para guardar el Serial y armar el Topic del Shadow ===
static char s_thing_name[32] = {0};

// ------------------------------------------------------------------
// HELPER CRÍTICO: Inyecta salto de línea y terminador nulo de forma segura
// ------------------------------------------------------------------
static char *prepare_pem(const uint8_t *raw_data, size_t actual_len)
{
    if (!raw_data || actual_len == 0)
        return NULL;

    // Reservamos espacio para los datos exactos + el '\n' + el '\0'
    char *safe_str = calloc(1, actual_len + 2);

    // Copiamos ESTRICTAMENTE los bytes válidos del certificado
    memcpy(safe_str, raw_data, actual_len);

    // Verificamos si el último carácter válido ya es un salto de línea
    if (safe_str[actual_len - 1] != '\n')
    {
        safe_str[actual_len] = '\n';
        safe_str[actual_len + 1] = '\0';
    }
    else
    {
        safe_str[actual_len] = '\0'; // Si ya lo tenía, solo cerramos el string
    }

    return safe_str;
}

// ------------------------------------------------------------------
// Eventos MQTT
// ------------------------------------------------------------------
// ------------------------------------------------------------------
// Eventos MQTT (Con Soporte para AWS Shadow Delta)
// ------------------------------------------------------------------
static void mqtt_event(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Conectado exitosamente a AWS IoT Core");
        s_is_connected = true;

        // SUSCRIPCIÓN AL SHADOW DELTA
        if (strlen(s_thing_name) > 0)
        {
            char delta_topic[128];
            snprintf(delta_topic, sizeof(delta_topic), "$aws/things/%s/shadow/update/delta", s_thing_name);
            int msg_id = esp_mqtt_client_subscribe(event->client, delta_topic, 1);
            ESP_LOGI(TAG, "Suscrito al Shadow Delta (ID: %d): %s", msg_id, delta_topic);
        }
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Desconectado de AWS");
        s_is_connected = false;
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "Suscripción confirmada por AWS. ¡Estamos a la escucha!");
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "Datos recibidos en el topic: %.*s", event->topic_len, event->topic);

        if (strstr(event->topic, "/shadow/update/delta") != NULL)
        {
            ESP_LOGW(TAG, "¡Cambio de configuración recibido desde la Nube!");

            char *payload = malloc(event->data_len + 1);
            if (payload)
            {
                memcpy(payload, event->data, event->data_len);
                payload[event->data_len] = '\0';

                cJSON *root = cJSON_Parse(payload);
                if (root)
                {
                    cJSON *state = cJSON_GetObjectItem(root, "state");
                    if (state)
                    {
                        AppConfig *tmp_cfg = malloc(sizeof(AppConfig));
                        if (tmp_cfg)
                        {
                            appcfg_load(tmp_cfg);
                            cfg_from_json(tmp_cfg, state);
                            appcfg_save(tmp_cfg);

                            ESP_LOGW(TAG, "=================================================");
                            ESP_LOGW(TAG, "NVS actualizada vía Shadow. Reiniciando equipo...");
                            ESP_LOGW(TAG, "=================================================");

                            // Liberamos la memoria antes de reiniciar
                            free(tmp_cfg);
                            cJSON_Delete(root);
                            free(payload);

                            // Esperamos 1.5 segundos para que los hilos de NVS vacíen los buffers a la Flash
                            vTaskDelay(pdMS_TO_TICKS(1500));

                            // REINICIO DE HARDWARE CONTROLADO POR SOFTWARE
                            esp_restart();
                        }
                    }
                    // Si el flujo continuara normalmente (no delta), se borraría aquí
                    if (root)
                        cJSON_Delete(root);
                }
                if (payload)
                    free(payload);
            }
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "Error MQTT (Revisa certificados o política de AWS)");
        break;
    default:
        break;
    }
}

// ------------------------------------------------------------------
// Tarea de Telemetría
// ------------------------------------------------------------------
static void aws_telemetry_task(void *pvParameters)
{
    char topic[256] = {0}; // Inicializamos vacío

    // 1. Extraemos de forma segura la configuración de tu NVS
    AppConfig *cfg_app = malloc(sizeof(AppConfig));

    // Valores de respaldo por si falla la lectura
    float p_lim_min = -10.0, p_lim_max = 120.0;
    float f_lim_min = 0.0, f_lim_max = 90.0;

    if (cfg_app)
    {
        appcfg_defaults(cfg_app);
        appcfg_load(cfg_app);

        // ENSAMBLE DEL TOPIC INTELIGENTE (Ruteo AWS)
        // Estructura: base / partner / client / model / serial / telemetry
        snprintf(topic, sizeof(topic), "%s/%s/%s/%s/%s/telemetry",
                 cfg_app->cloud.topic_base,
                 cfg_app->general.partner,
                 cfg_app->general.client,
                 cfg_app->general.model,
                 cfg_app->general.serial);

        ESP_LOGI(TAG, "Topic MQTT configurado: %s", topic);

        // Extraemos los límites de alarma reales de la NVS para evaluarlos en tiempo real
        p_lim_min = cfg_app->sensors.alarm_limits.pressure_min;
        p_lim_max = cfg_app->sensors.alarm_limits.pressure_max;
        // Si a futuro agregas límites fijos de flujo a la estructura, los cargas aquí:
        // f_lim_min = cfg_app->sensors.alarm_limits.flow_min;

        free(cfg_app); // Liberamos los bytes del Heap de inmediato
    }
    else
    {
        // Fallback de emergencia si no hay RAM
        strcpy(topic, "vexel/unknown/telemetry");
    }

    while (1)
    {
        // Publicación periódica cada 5 segundos
        vTaskDelay(pdMS_TO_TICKS(5000));

        if (!s_is_connected)
            continue;

        // 2. Extracción de variables REALES del módulo runtime de sensores
        sensor_sample_t current_sample;
        sensor_sample_t min_sample;
        sensor_sample_t max_sample;

        // sensors_runtime_get_last obtiene la última muestra agregada por la tarea dummy
        bool have_last = sensors_runtime_get_last(&current_sample);
        // sensors_runtime_get_min_max calcula el histórico real en una ventana de tiempo (ej. últimos 30 seg)
        bool have_stats = sensors_runtime_get_min_max(30000, &min_sample, &max_sample);

        if (!have_last)
        {
            ESP_LOGW(TAG, "Esperando datos válidos del runtime de sensores...");
            continue;
        }

        // Si no hay ventana estadística acumulada aún, usamos el valor actual como respaldo
        float p_min = have_stats ? min_sample.pressure_kpa : current_sample.pressure_kpa;
        float p_max = have_stats ? max_sample.pressure_kpa : current_sample.pressure_kpa;
        float f_min = have_stats ? min_sample.flow_lpm : current_sample.flow_lpm;
        float f_max = have_stats ? max_sample.flow_lpm : current_sample.flow_lpm;

        // Cálculo del promedio matemático (Average) de la ventana actual
        float p_avg = (p_min + p_max) / 2.0f;
        float f_avg = (f_min + f_max) / 2.0f;

        // 3. Construcción del JSON Arquetípico de Vexel (idéntico a tu Dashboard)
        cJSON *root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "timestamp", (double)(esp_timer_get_time() / 1000));
        cJSON_AddStringToObject(root, "device_status", "OPERATIONAL");

        // --- SUB-OBJETO: PRESION ---
        cJSON *pres = cJSON_CreateObject();
        cJSON_AddNumberToObject(pres, "current", current_sample.pressure_kpa);
        cJSON_AddNumberToObject(pres, "min", p_min);
        cJSON_AddNumberToObject(pres, "max", p_max);
        cJSON_AddNumberToObject(pres, "avg", p_avg);
        cJSON_AddItemToObject(root, "pressure", pres);

        // --- SUB-OBJETO: FLUJO ---
        cJSON *flow = cJSON_CreateObject();
        cJSON_AddNumberToObject(flow, "current", current_sample.flow_lpm);
        cJSON_AddNumberToObject(flow, "min", f_min);
        cJSON_AddNumberToObject(flow, "max", f_max);
        cJSON_AddNumberToObject(flow, "avg", f_avg);
        cJSON_AddItemToObject(root, "flow", flow);

        // --- SUB-OBJETO: ALARMAS (Evaluación industrial en tiempo real) ---
        cJSON *alarms = cJSON_CreateObject();
        cJSON_AddBoolToObject(alarms, "pressure_low", (current_sample.pressure_kpa < p_lim_min));
        cJSON_AddBoolToObject(alarms, "pressure_high", (current_sample.pressure_kpa > p_lim_max));
        cJSON_AddBoolToObject(alarms, "flow_low", (current_sample.flow_lpm < f_lim_min));
        cJSON_AddBoolToObject(alarms, "flow_high", (current_sample.flow_lpm > f_lim_max));
        cJSON_AddItemToObject(root, "alarms", alarms);

        // 4. Serialización y Publicación en AWS IoT Core
        char *pub_payload = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);

        if (pub_payload)
        {
            int msg_id = esp_mqtt_client_publish(s_client, topic, pub_payload, 0, 1, 0);
            if (msg_id >= 0)
            {
                ESP_LOGI(TAG, "Telemetría enviada a AWS con éxito. ID: %d", msg_id);
            }
            else
            {
                ESP_LOGE(TAG, "Fallo al publicar telemetría en el broker.");
            }
            free(pub_payload); // Evitamos fugas de memoria en el Heap interno
        }
    }
}

// ------------------------------------------------------------------
// Inicialización Segura
// ------------------------------------------------------------------
void transport_mqtt_on_time_ready(void)
{
    if (s_client)
    {
        ESP_LOGI(TAG, "MQTT ya estaba inicializado.");
        return;
    }

    // 1. Extraemos los certificados crudos desde el almacén "certs" de la NVS
    uint8_t *root_ca = NULL, *client_cert = NULL, *client_key = NULL;
    size_t r_len = 0, c_len = 0, k_len = 0;

    if (cert_store_load(CERT_SERVER_CA, &root_ca, &r_len) != ESP_OK ||
        cert_store_load(CERT_CLIENT_CERT, &client_cert, &c_len) != ESP_OK ||
        cert_store_load(CERT_CLIENT_KEY, &client_key, &k_len) != ESP_OK)
    {
        ESP_LOGE(TAG, "Error crítico: No se pudieron cargar los certificados desde cert_store");
        if (root_ca)
            free(root_ca);
        if (client_cert)
            free(client_cert);
        if (client_key)
            free(client_key);
        return;
    }

    // 2. Cargamos tu configuración general (identidad, urls, etc) desde el Heap
    AppConfig *cfg_app = malloc(sizeof(AppConfig));
    if (!cfg_app)
    {
        ESP_LOGE(TAG, "No hay memoria suficiente para AppConfig");
        free(root_ca);
        free(client_cert);
        free(client_key);
        return;
    }
    appcfg_defaults(cfg_app);
    appcfg_load(cfg_app);
    // === NUEVO: Guardamos el serial en RAM para usarlo en los eventos MQTT ===
    strncpy(s_thing_name, cfg_app->general.serial, sizeof(s_thing_name) - 1);

    // 3. Preparamos las cadenas PEM seguras (Inyectando el \n si falta)
    //    Las guardamos en las variables estáticas globales del archivo para que NO se destruyan
    if (!s_safe_ca)
        s_safe_ca = prepare_pem(root_ca, r_len);
    if (!s_safe_cert)
        s_safe_cert = prepare_pem(client_cert, c_len);
    if (!s_safe_key)
        s_safe_key = prepare_pem(client_key, k_len);

    // Liberamos los buffers intermedios que ya duplicamos de forma segura
    free(root_ca);
    free(client_cert);
    free(client_key);

    // =========================================================
    // TU LOG DE DEBUG FORENSE (Mantenido intacto para verificar)
    // =========================================================
    ESP_LOGE(TAG, "========= DEBUG CERTIFICADO CA =========");
    size_t final_ca_len = strlen(s_safe_ca);
    ESP_LOGE(TAG, "Longitud original de cert_store: %d", r_len);
    ESP_LOGE(TAG, "Longitud final calculada (strlen): %d", final_ca_len);
    ESP_LOGE(TAG, "Ultimos 25 caracteres en texto:");
    printf("-->%s<--\n", s_safe_ca + (final_ca_len > 25 ? final_ca_len - 25 : 0));
    ESP_LOGE(TAG, "Ultimos 25 bytes en HEX (Buscando basura o falta de \\n):");
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, s_safe_ca + (final_ca_len > 25 ? final_ca_len - 25 : 0), 25, ESP_LOG_ERROR);
    ESP_LOGE(TAG, "========================================");
    // =========================================================

    // 4. Configuramos el motor MQTT apuntando a las variables estáticas s_safe_...
    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = cfg_app->cloud.broker_url,
        .credentials.client_id = cfg_app->general.serial,
        .credentials.authentication.certificate = s_safe_cert,
        .credentials.authentication.key = s_safe_key,
        .broker.verification.certificate = s_safe_ca};

    s_client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event, NULL);

    // Lanzamos el cliente
    esp_mqtt_client_start(s_client);

    // Devolvemos los 6.5 KB de la estructura de configuración inmediatamente al Heap
    free(cfg_app);

    // Bajamos el stack a 4096 bytes (1024 palabras), suficiente para cJSON.
    xTaskCreatePinnedToCore(aws_telemetry_task, "aws_pub", 4096, NULL, 5, NULL, 1);
}

void transport_mqtt_on_net_down(void) {}
bool cloud_mgr_connected(void) { return s_is_connected; }
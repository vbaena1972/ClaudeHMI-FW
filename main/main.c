#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_vfs_fat.h"
#include "esp_heap_trace.h"
#include "esp_heap_caps.h"
#include "esp_app_trace.h"
//#include "segger_sysview.h"
#include "esp_trace.h"

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "nvs_flash.h"
#include "pinmap.h"
#include "lvgl.h"
#include "ui.h"
#include "ui_sd.h"
#include "driver/i2c_master.h"
#include "driver/sdmmc_host.h"
#include "driver/gpio.h"
#include "ft5x06.h"
#include "st7796.h"

#include "sdmmc_cmd.h"
#include "mem_diag.h"
#include "task_tracer.h"
#include "ui_wifi_main_icon.h"
#include "ui_statusbar_controller.h"
#include "wifi_mgr.h"
#include "eth_mgr.h"
#include "storage.h"
#include "metrics_store.h"
#include "net_core.h"
#include "cert_store.h"
#include "sensors_runtime.h"
#include "transport_ble.h"
#include "state_pub.h"
#include "http_api.h"
#include "lwip/ip_addr.h"
#include "rtc_rv3028.h"
#include "alarm_mgr.h"
#include "time_mgr.h"

// #define SD_CD_PIN 42
//  Tareas de Procesamiento de Datos (Workers)
#define PRIO_BLE_WORKER 4 // Procesar JSON rápido, pero dejar renderizar a LVGL

// Tareas de Lógica de Interfaz (Refresco)
#define PRIO_UI_LOGIC 2 // BAJA. Debe ser MENOR que la tarea de LVGL para evitar deadlock.

// Tareas de Inicialización (Corren una vez y mueren/suspenden)
#define PRIO_INIT_TASK 3 // Media.

//#define MY_TASK_STACK_WORDS 16384
#define MY_TASK_STACK_WORDS (16384 / sizeof(StackType_t))

#define BLE_JSON_MAX 1024
#define BLE_TASK_STACK_SIZE 4096

typedef struct
{
    size_t len;
    char *buf;
} ble_json_msg_t;

static QueueHandle_t s_ble_q = NULL;
typedef struct
{
    TaskHandle_t handle;
    StackType_t *stack_buf;
    StaticTask_t *tcb_buf;
} init_task_ctx_t;

static init_task_ctx_t screen_task_ctx;
static init_task_ctx_t eth_task_ctx;
static init_task_ctx_t wifi_task_ctx;
static init_task_ctx_t ble_task_ctx;
static EventGroupHandle_t init_events;
#define EVT_SCREEN_DONE (1 << 0)
#define EVT_WIFI_INIT_DONE (1 << 1)
#define EVT_ETH_INIT_DONE (1 << 2)
#define EVT_BT_INIT_DONE (1 << 3)
// Variable global para guardar la referencia de la tarjeta (necesaria para desmontarla luego)
sdmmc_card_t *s_sd_card = NULL;

i2c_master_bus_handle_t bus_handle;
static const char *TAG = "app";
extern void ui_wifi_main_icon_init(lv_obj_t *img_obj);

static bool wifi_cfg_valid = false;

/* I2C */
#define BSP_I2C_SCL (GPIO_NUM_15)
#define BSP_I2C_SDA (GPIO_NUM_16)
#define BSP_I2C_PORT 1

// Definición de la macro de seguridad: Si el objeto es NULL, salta y evita un potencial crash.
#define LV_OBJ_GUARD(obj)                                        \
    if ((obj) == NULL)                                           \
    {                                                            \
        ESP_LOGW("ui_refresh", "LVGL Object is NULL: %s", #obj); \
        vTaskDelay(pdMS_TO_TICKS(10));                           \
        continue;                                                \
    }

static SemaphoreHandle_t s_sd_sem = NULL;
/* La UI del flujo SD vive ahora en main/ui/ui_sd.c (overlay modal del design system). */

static heap_trace_record_t trace_record[400];

static void ui_refresh_task(void *arg);

// ISR (Manejador de la interrupción por hardware)
static void IRAM_ATTR sd_cd_isr_handler(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Si el pin pasa a bajo (0) significa que la tarjeta entró (comportamiento común de SD_CD)
    ESP_LOGI("SD_CD", "Interrupcion por microSD intsertada...");
    if (gpio_get_level(SD_CD_PIN) == 0)
    {
        xSemaphoreGiveFromISR(s_sd_sem, &xHigherPriorityTaskWoken);
    }
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

// Evento del botón de reinicio en LVGL
static void restart_btn_event_cb(lv_event_t *e)
{
    ESP_LOGW("MAIN", "Reiniciando equipo por confirmación del usuario...");
    esp_restart();
}

// Función obligatoria en ESP-IDF v6 para enlazar SystemView y AppTrace
esp_trace_open_params_t esp_trace_get_user_params(void)
{
    esp_trace_open_params_t trace_params = {
        .core_cfg = NULL,
        .encoder_name = "sysview", // <--- Esto corrige el error 'ext'
        .encoder_cfg = NULL,
        .transport_name = "apptrace",
        .transport_cfg = NULL,
    };
    return trace_params;
}

void trace_init(void)
{
    ESP_ERROR_CHECK(heap_trace_init_standalone(trace_record,
                                               sizeof(trace_record) / sizeof(trace_record[0])));
}

static void ble_json_worker(void *arg)
{
    ble_json_msg_t m;
    while (xQueueReceive(s_ble_q, &m, portMAX_DELAY) == pdTRUE)
    {
        if (m.buf && m.len)
        {
            // Procesa AQUÍ, ya fuera del BTC_TASK
            (void)appcfg_patch(-1, m.buf, "ble");
            free(m.buf);
        }
    }
}

static void on_ble_json_shim(const char *buf, int len)
{
    if (!buf || len <= 0 || len > BLE_JSON_MAX)
        return;

    char *p = malloc(len + 1);
    if (!p)
        return;
    memcpy(p, buf, len);
    p[len] = '\0';

    ble_json_msg_t m = {.len = (size_t)len, .buf = p};
    if (xQueueSend(s_ble_q, &m, 0) != pdTRUE)
    {
        free(p);
    }
}

static esp_err_t bsp_i2c_init(void)
{
    i2c_master_bus_config_t cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = BSP_I2C_PORT,
        .scl_io_num = BSP_I2C_SCL,
        .sda_io_num = BSP_I2C_SDA,
        .glitch_ignore_cnt = 1,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &bus_handle));
    ESP_LOGI(TAG, "I2C master listo");
    return ESP_OK;
}

void on_network_ready(void)
{
    ESP_ERROR_CHECK(http_api_init());
}

esp_err_t mount_sdcard_hotplug(void)
{
    ESP_LOGI("SD_CD", "Inicializando hardware SD...");

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.flags = SDMMC_HOST_FLAG_1BIT;

    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 1;
#if SOC_SDMMC_USE_GPIO_MATRIX
    slot.clk = 45;
    slot.cmd = 0;
    slot.d0 = 21;
#endif

    const esp_vfs_fat_sdmmc_mount_config_t mnt = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 0,
        .disk_status_check_enable = true, // Ideal para sistemas Hot-Plug
    };

    return esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot, &mnt, &s_sd_card);
}

void unmount_sdcard_hotplug(void)
{
    if (s_sd_card)
    {
        esp_vfs_fat_sdcard_unmount("/sdcard", s_sd_card);
        s_sd_card = NULL;
        ESP_LOGI("SD_CD", "Tarjeta desmontada de forma segura.");
    }
}

static void sd_monitor_task(void *pvParameters)
{
    s_sd_sem = xSemaphoreCreateBinary();

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << SD_CD_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_NEGEDGE // Flanco de bajada
    };
    gpio_config(&io_conf);

    // =========================================================
    // INSTALACIÓN SEGURA DEL SERVICIO ISR
    // =========================================================
    esp_err_t isr_err = gpio_install_isr_service(0);
    // Si da ESP_ERR_INVALID_STATE significa que ya estaba instalado, lo ignoramos.
    if (isr_err != ESP_OK && isr_err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE("SD_CD", "Fallo al instalar servicio ISR: %s", esp_err_to_name(isr_err));
    }

    gpio_isr_handler_add(SD_CD_PIN, sd_cd_isr_handler, NULL);
    // =========================================================

    while (1)
    {
        if (xSemaphoreTake(s_sd_sem, portMAX_DELAY) == pdTRUE)
        {
            // 1. DEBOUNCE: Esperamos 500ms a que pase el ruido eléctrico de la inserción
            vTaskDelay(pdMS_TO_TICKS(500));
            if (gpio_get_level(SD_CD_PIN) != 0)
                continue; // Falso contacto, abortar

            ESP_LOGW("SD_CD", "¡Tarjeta MicroSD insertada (Hot-Plug)!");

            // 2. MONTAJE: ¡AFUERA DEL MUTEX!
            esp_err_t r = mount_sdcard_hotplug();
            if (r != ESP_OK)
            {
                ESP_LOGE("SD_CD", "Fallo al inicializar SD. Error: %d (%s)", r, esp_err_to_name(r));
                continue; // Cancelar todo y seguir esperando
            }
            else
            {
                ESP_LOGI("SD_CD", "Sistema de archivos de la SD inicializado con éxito.");
            }

            // 3. UI: ¡ADENTRO DEL MUTEX!
            if (bsp_display_lock(portMAX_DELAY))
            {
                ui_sd_show();
                ui_sd_progress(10, "Tarjeta detectada…");
                bsp_display_unlock();
            }

            bool debede_actualizar = false;

            // 4. LECTURA DE ARCHIVOS: ¡AFUERA DEL MUTEX!
            DIR *dir = opendir("/sdcard");
            if (dir)
            {
                struct dirent *ent;
                while ((ent = readdir(dir)) != NULL)
                {
                    ESP_LOGI("SD_CD", "Archivo SD: %s", ent->d_name);
                }
                closedir(dir);
            }

            FILE *f_cfg = fopen("/sdcard/AppConfig.json", "rb");
            if (f_cfg)
            {
                if (bsp_display_lock(portMAX_DELAY))
                {
                    ui_sd_progress(50, "Aplicando AppConfig.json…");
                    bsp_display_unlock();
                }

                fseek(f_cfg, 0, SEEK_END);
                long size = ftell(f_cfg);
                fseek(f_cfg, 0, SEEK_SET);

                char *buf = malloc(size + 1);
                if (buf)
                {
                    size_t bytes_read = fread(buf, 1, size, f_cfg);
                    buf[bytes_read] = '\0';

                    char *json_ptr = buf;
                    // Sanitizador BOM UTF-8
                    if (bytes_read >= 3 && (unsigned char)json_ptr[0] == 0xEF && (unsigned char)json_ptr[1] == 0xBB && (unsigned char)json_ptr[2] == 0xBF)
                    {
                        json_ptr += 3;
                    }

                    // Sanitizador de Espacios Duros
                    char *r_ptr = json_ptr, *w_ptr = json_ptr;
                    while (*r_ptr)
                    {
                        if ((unsigned char)*r_ptr == 0xC2 && (unsigned char)*(r_ptr + 1) == 0xA0)
                        {
                            *w_ptr++ = ' ';
                            r_ptr += 2;
                        }
                        else if ((unsigned char)*r_ptr == 0xA0)
                        {
                            *w_ptr++ = ' ';
                            r_ptr++;
                        }
                        else
                        {
                            *w_ptr++ = *r_ptr++;
                        }
                    }
                    *w_ptr = '\0';

                    AppConfig *temp_cfg = malloc(sizeof(AppConfig));
                    if (temp_cfg)
                    {
                        appcfg_load(temp_cfg);
                        cJSON *root = cJSON_Parse(json_ptr);
                        if (root)
                        {
                            cfg_from_json(temp_cfg, root);
                            appcfg_save(temp_cfg);
                            cJSON_Delete(root);
                            debede_actualizar = true;
                            ESP_LOGI("SD_CD", "NVS AppConfig actualizada con éxito.");
                        }
                        else
                        {
                            ESP_LOGE("SD_CD", "Fallo cJSON_Parse. Error: [%s]", cJSON_GetErrorPtr());
                        }
                        free(temp_cfg);
                    }
                    free(buf);
                }
                fclose(f_cfg);
                rename("/sdcard/AppConfig.json", "/sdcard/AppConfig.bak");
            }

            if (bsp_display_lock(portMAX_DELAY))
            {
                ui_sd_progress(70, "Importando certificados AWS…");
                bsp_display_unlock();
            }

            // 5. IMPORTAR CERTIFICADOS: ¡AFUERA DEL MUTEX!
            if (cert_store_import_from_sd("/sdcard/aws") == ESP_OK)
            {
                debede_actualizar = true;
                ESP_LOGI("SD_CD", "Certificados procesados.");
            }

            if (bsp_display_lock(portMAX_DELAY))
            {
                ui_sd_progress(100, "Completado");
                bsp_display_unlock();
            }
            vTaskDelay(pdMS_TO_TICKS(500));

            // 6. CONCLUSIÓN (UI ADENTRO DEL MUTEX, DESMONTAJE AFUERA)
            if (debede_actualizar)
            {
                if (bsp_display_lock(portMAX_DELAY))
                {
                    ui_sd_finish_restart(restart_btn_event_cb);
                    bsp_display_unlock();
                }
                ESP_LOGW("SD_CD", "Esperando confirmación táctil para reiniciar.");
            }
            else
            {
                if (bsp_display_lock(portMAX_DELAY))
                {
                    ui_sd_close();
                    bsp_display_unlock();
                }
                ESP_LOGI("SD_CD", "No se actualizó nada. Desmontando tarjeta...");
                unmount_sdcard_hotplug(); // <-- Desmontar tarjeta si no hay reinicio
            }
        }
    }
}

static void ui_main_deinit_cb(lv_event_t *e)
{
    ui_wifi_main_icon_deinit();
    ui_statusbar_controller_deinit();
}

static void screen_init_task(void *arg)
{
    (void)arg;
    UBaseType_t hwm_start = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI("screen_init", "Stack HWM at start (words): %u", (unsigned)hwm_start);

    ESP_LOGI(TAG, "Starting screen initialization");

    if (bsp_i2c_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize I2C");
        if (init_events)
        {
            xEventGroupSetBits(init_events, EVT_SCREEN_DONE);
        }
        vTaskDelete(NULL);
        return;
    }
    else
    {
        ESP_LOGI(TAG, "I2C initialized successfully");
    }
    // Dentro de screen_init_task, justo después de bsp_i2c_init():
    rtc_rv3028_init(bus_handle); // Le pasamos el bus I2C que ya tienes creado
    time_mgr_init();             // Intenta cargar la hora local

    lv_display_t *disp = bsp_display_start();
    if (!disp)
    {
        ESP_LOGE(TAG, "Failed to initialize display");
        if (init_events)
        {
            xEventGroupSetBits(init_events, EVT_SCREEN_DONE);
        }
        vTaskSuspend(NULL);
        return;
    }
    else
    {
        ESP_LOGI(TAG, "Display initialized successfully");
    }

    (void)bsp_display_indev_init(disp, bus_handle);
    // bsp_display_rotate(disp, LV_DISP_ROTATION_270);

    // if (bsp_display_lock(portMAX_DELAY) == ESP_OK)
    // {
    //     ESP_LOGI("Display Init Task", "Locking display for initialization");
    // }else
    // {
    //     ESP_LOGE("Display Init Task", "FALLO CRITICO: No se pudo adquirir el mutex para init.");
    // }

    // ROTACIÓN: Hacerla ANTES de cualquier otra cosa gráfica
    // Nota: bsp_display_rotate usa lv_disp_set_rotation.
    // INTENTA BLOQUEAR ANTES DE ROTAR
    if (bsp_display_lock(portMAX_DELAY))
    {
        bsp_display_rotate(disp, LV_DISP_ROTATION_270);
        bsp_display_unlock();
    }
    else
    {
        ESP_LOGE("screen_init", "No se pudo bloquear para rotación");
    }

    // UI INIT
    // El problema principal: ui_init() crea widgets. ESTO REQUIERE LOCK.
    // Si el lock falla aquí, la UI no se creará correctamente o corromperá la memoria de LVGL.

    ESP_LOGI("screen_init", "Intentando bloquear para ui_init...");
    if (bsp_display_lock(portMAX_DELAY))
    {
        ESP_LOGI("screen_init", "Mutex adquirido. Iniciando UI.");

        bsp_display_off(); // Apagar display para evitar parpadeo visual
        bsp_display_backlight_off();

        ui_init(); // <--- ESTO ES LO QUE CREA LOS LABELS
        ESP_LOGI(TAG, "ui_init - OK");

        extern lv_obj_t *ui_wifiStatusMain;
        ui_wifi_main_icon_init(ui_wifiStatusMain);
        ui_statusbar_controller_init();

        bsp_display_unlock();
        ESP_LOGI("screen_init", "Mutex liberado.");
    }
    else
    {
        ESP_LOGE("screen_init", "FALLO TOTAL: No se pudo bloquear para ui_init. LA UI NO SE MOSTRARÁ CORRECTAMENTE.");
        // Intentar ui_init sin lock como último recurso desesperado
        abort();
    }

    bsp_display_brightness_set(10);
    bsp_display_backlight_on();
    vTaskDelay(pdMS_TO_TICKS(10));
    {
        /* Brillo configurado por el usuario (general.brightness, saneado en appcfg_migrate) */
        const AppConfig *bcfg = appcfg_cache_peek();
        bsp_display_brightness_set(bcfg ? bcfg->general.brightness : 100);
    }
    bsp_display_on();
    ESP_LOGI(TAG, "UI iniciada");

    UBaseType_t hwm_end = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI("screen_init", "Stack HWM before delete (words): %u", (unsigned)hwm_end);

    xEventGroupSetBits(init_events, EVT_SCREEN_DONE);

    vTaskSuspend(NULL);
}

static void ble_init_task(void *arg)
{
    const AppConfig *cfg = (const AppConfig *)arg;

    UBaseType_t hwm_start = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI("ble_init", "Stack HWM at start (words): %u", (unsigned)hwm_start);

    esp_err_t e = transport_ble_init(true);
    if (e != ESP_OK)
    {
        ESP_LOGE("ble_init", "init BLE: %s", esp_err_to_name(e));
        ESP_LOGE("ble_init", "init BLE failed");
        xEventGroupSetBits(init_events, EVT_BT_INIT_DONE);
        vTaskSuspend(NULL);
        return; // <-- NECESARIO
    }

    transport_ble_set_cmd_handler(on_ble_json_shim);

    ESP_ERROR_CHECK(transport_ble_set_enabled(cfg->bt.enabled));

    if (cfg->bt.legacy.name[0])
    {
        ESP_ERROR_CHECK(transport_ble_set_name(cfg->bt.legacy.name));
    }

    bool use_passkey = (cfg->bt.legacy.sec_mode == 1 /* APP_BT_SEC_PASSKEY */) && cfg->bt.legacy.pin[0];
    uint32_t pk = 0;
    if (use_passkey)
    {
        pk = (uint32_t)strtoul(cfg->bt.legacy.pin, NULL, 10);
    }
    ESP_ERROR_CHECK(transport_ble_set_passkey_mode(use_passkey, pk));

    ESP_ERROR_CHECK(transport_ble_set_tx_power(cfg->bt.tx_power));
    if (cfg->bt.advertise)
        ESP_ERROR_CHECK(transport_ble_start_adv());
    else
        ESP_ERROR_CHECK(transport_ble_stop_adv());

    UBaseType_t hwm_end = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI("ble_init", "Stack HWM before delete (words): %u", (unsigned)hwm_end);

    if (init_events)
    {
        xEventGroupSetBits(init_events, EVT_BT_INIT_DONE);
        ESP_LOGW("ble_init", "Entro aqui 2");
    }

    vTaskSuspend(NULL);
}

static void eth_init_task(void *arg)
{
    // const AppConfig *cfg = (const AppConfig *)arg;

    UBaseType_t hwm_start = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI("eth_init", "Stack HWM at start (words): %u", (unsigned)hwm_start);

    esp_err_t r = eth_mgr_init();
    if (r != ESP_OK)
    {
        ESP_LOGE("eth_init", "eth_mgr_init failed: %s", esp_err_to_name(r));
        if (init_events)
        {
            xEventGroupSetBits(init_events, EVT_ETH_INIT_DONE);
            ESP_LOGW("eth_init", "Entro aqui");
        }
        vTaskSuspend(NULL);
        return;
    }

    esp_err_t r2 = eth_mgr_start();
    if (r2 != ESP_OK)
    {
        ESP_LOGE("eth_init", "eth_mgr_start failed: %s", esp_err_to_name(r2));
    }

    UBaseType_t hwm_end = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI("eth_init", "Stack HWM before suspend (words): %u", (unsigned)hwm_end);

    if (init_events)
    {
        xEventGroupSetBits(init_events, EVT_ETH_INIT_DONE);
        ESP_LOGW("eth_init", "Entro aqui 2");
    }

    vTaskSuspend(NULL);
}

static void wifi_init_task(void *arg)
{
    const AppConfig *cfg = (const AppConfig *)arg;

    UBaseType_t hwm_start = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI("wifi_init", "Stack HWM at start (words): %u", (unsigned)hwm_start);

    esp_err_t e = wifi_mgr_init();
    if (e != ESP_OK)
    {
        ESP_LOGE("wifi_init", "wifi_mgr_init failed: %s", esp_err_to_name(e));
        if (init_events)
        {
            xEventGroupSetBits(init_events, EVT_WIFI_INIT_DONE);
        }
        vTaskSuspend(NULL); // o simplemente "return;" si prefieres
        return;
    }

    if (cfg != NULL && wifi_cfg_valid)
    {
        ESP_ERROR_CHECK(wifi_mgr_apply_from_cache());
    }

    ESP_ERROR_CHECK(wifi_mgr_start());

    UBaseType_t hwm_end = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI("wifi_init", "Stack HWM before delete (words): %u", (unsigned)hwm_end);
    if (init_events)
    {
        xEventGroupSetBits(init_events, EVT_WIFI_INIT_DONE);
    }
    vTaskSuspend(NULL);
}

static void ui_refresh_task(void *arg)
{
    const AppConfig *cfg = (const AppConfig *)arg;
    (void)cfg;   /* ya no se usa: ui_main_update lee el caché vivo */

    ESP_LOGI("ui_refresh", "ui_refresh_task START");

    sensor_sample_t last, min_s, max_s;
    char clockbuf[8];
    time_t last_clock_min = -1;

    /* --- Métricas persistentes (consumo del día + minutos de servicio) --- */
    app_metrics_t metrics;
    appmetrics_load(&metrics);
    bool metrics_seeded = false;      /* siembra del consumo pendiente hasta tener fecha válida */
    time_t last_metrics_save = 0;

    while (true)
    {
        bool have_last = sensors_runtime_get_last(&last);
        bool have_mm = sensors_runtime_get_min_max(60000, &min_s, &max_s);

        if (!have_last && !have_mm)
        {
            vTaskDelay(pdMS_TO_TICKS(300));
            continue;
        }

        // Alimentar la máquina de alarmas con las muestras en vivo
        if (have_last)
            alarm_mgr_process(last.pressure_kpa, last.flow_lpm);
        alarm_clinical_state_t current_state = alarm_mgr_get_current_state();
        bool is_muted = alarm_mgr_is_muted();

        // Refrescar la HMI: todo el manejo de widgets vive en la capa UI (ui_main_update)
        if (bsp_display_lock(pdMS_TO_TICKS(100)))
        {
            /* NULL -> ui_main_update usa el AppConfig del caché VIVO (appcfg_cache_peek),
               así los cambios de unidad/umbral hechos desde la UI se reflejan sin reiniciar. */
            ui_main_update(&last, have_last, &min_s, &max_s, have_mm,
                           NULL, current_state, is_muted);

            // Reloj de la cabecera (se actualiza al cambiar de minuto)
            time_t now = time(NULL);
            if (now / 60 != last_clock_min)
            {
                struct tm tm_now;
                localtime_r(&now, &tm_now);
                strftime(clockbuf, sizeof(clockbuf), "%H:%M", &tm_now);
                ui_main_set_clock(clockbuf);
                last_clock_min = now / 60;

                /* --- Métricas: siembra al arrancar + guardado cada 10 min --- */
                char today[12];
                strftime(today, sizeof(today), "%Y-%m-%d", &tm_now);
                if (!metrics_seeded)
                {
                    /* si lo guardado es de HOY, restaurar el consumo tras el reinicio */
                    if (strcmp(metrics.date, today) == 0)
                        ui_main_set_consumo(metrics.consumo_m3);
                    metrics_seeded = true;
                    last_metrics_save = now;
                }
                else if (now - last_metrics_save >= 600)
                {
                    metrics.service_min += (uint32_t)((now - last_metrics_save) / 60);
                    strncpy(metrics.date, today, sizeof(metrics.date) - 1);
                    metrics.date[sizeof(metrics.date) - 1] = '\0';
                    metrics.consumo_m3 = ui_main_get_consumo();
                    (void)appmetrics_store(&metrics);
                    last_metrics_save = now;
                }
            }

            if (ui_mainScreen)
                lv_obj_invalidate(ui_mainScreen);
            bsp_display_unlock();
        }
        else
        {
            ESP_LOGW("ui_refresh", "Mutex ocupado (Timeout 100ms).");
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void app_main(void)
{

    // --- TRAMPA PARA EL LINKER ---
    // Forzamos al compilador a inyectar la telemetría en el binario final
    // volatile void *dummy_trace = (void *)esp_apptrace_init;
    // volatile void *dummy_sysview = (void *)SEGGER_SYSVIEW_Start;
    // (void)dummy_trace;
    // (void)dummy_sysview;
    // -----------------------------

    mem_diag_report("BOOT-START");

    // NVS primero
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    mem_diag_report("AFTER-NVS");

    // ... tras nvs_flash_init() OK
    extern esp_err_t appcfg_cache_reload(void);
    appcfg_cache_reload(); // carga NVS -> DRAM (stack interno de main), seguro

    /* Zona horaria configurada -> TZ del RTC (localtime en ui_refresh_task) */
    {
        extern void ui_cfg_apply_timezone(const char *iana);
        const AppConfig *tzc = appcfg_cache_peek();
        if (tzc) ui_cfg_apply_timezone(tzc->general.timezone);
    }
    mem_diag_report("AFTER-APPCFG-RELOAD");
    // LVGL init
    lv_init();
    vTaskDelay(pdMS_TO_TICKS(1000));
    mem_diag_report("AFTER-LV-INIT");

    alarm_mgr_init(BUZZER_PWM_GPIO);

    // Estado público (MQTT, BLE, etc)
    ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t er = esp_event_loop_create_default();
    if (er != ESP_OK && er != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(er);
    }
    net_core_init();
    on_network_ready(); // adentro hace http_api_init()
    mem_diag_report("AFTER-STATE-PUB");

    // Monta microSD (para import/borrado de certs desde UI_CloudCfg)
    xTaskCreate(sd_monitor_task, "sd_monitor", 4096, NULL, 5, NULL);
    mem_diag_report("AFTER-SDCARD");

    init_events = xEventGroupCreate();
    if (init_events == NULL)
    {
        ESP_LOGE("app", "No se pudo crear init_events");
        abort();
    }

    // I2C (touch) y Display
    //screen_task_ctx.stack_buf = heap_caps_malloc(16384, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    screen_task_ctx.stack_buf = heap_caps_malloc(MY_TASK_STACK_WORDS * sizeof(StackType_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    screen_task_ctx.tcb_buf = heap_caps_malloc(sizeof(StaticTask_t), MALLOC_CAP_INTERNAL);
    assert(screen_task_ctx.stack_buf && screen_task_ctx.tcb_buf);

    screen_task_ctx.handle = xTaskCreateStaticPinnedToCore(
        screen_init_task,
        "screen_init",
        MY_TASK_STACK_WORDS,
        NULL,
        PRIO_INIT_TASK,
        screen_task_ctx.stack_buf,
        screen_task_ctx.tcb_buf,
        1);

    if (screen_task_ctx.handle == NULL)
    {
        ESP_LOGE("screen_init", "No se pudo crear screen_init task");
    }
    else
    {
        ESP_LOGI("screen_init", "screen_init task creada");
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    xEventGroupWaitBits(init_events, EVT_SCREEN_DONE,
                        pdTRUE,
                        pdFALSE,
                        portMAX_DELAY);
    vTaskDelete(screen_task_ctx.handle);
    heap_caps_free(screen_task_ctx.stack_buf);
    heap_caps_free(screen_task_ctx.tcb_buf);
    screen_task_ctx.handle = NULL;
    screen_task_ctx.stack_buf = NULL;
    screen_task_ctx.tcb_buf = NULL;
    ESP_LOGW("screen_init", "Done!");
    mem_diag_report("AFTER-LVGL");

    static AppConfig cfg;

    sensors_runtime_init(&cfg);
    mem_diag_report("AFTER-SENSORS-RUNTIME");

    // *** BLUETOOTH ***//
    if (appcfg_cache_get(&cfg) == ESP_OK && cfg.bt.enabled)
    {
        s_ble_q = xQueueCreate(4, sizeof(ble_json_msg_t));

        // ASIGNAMOS DIRECTAMENTE AL CONTEXTO GLOBAL PARA PODER LIBERARLO DESPUÉS
        ble_task_ctx.stack_buf = (StackType_t *)heap_caps_malloc(BLE_TASK_STACK_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        ble_task_ctx.tcb_buf = (StaticTask_t *)heap_caps_malloc(sizeof(StaticTask_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

        if (ble_task_ctx.stack_buf != NULL && ble_task_ctx.tcb_buf != NULL)
        {
            // ¡AQUÍ ESTÁ LA MAGIA! Guardamos el handle.
            ble_task_ctx.handle = xTaskCreateStaticPinnedToCore(
                ble_init_task,
                "ble_init",
                BLE_TASK_STACK_SIZE / sizeof(StackType_t), // FreeRTOS estático espera palabras, no bytes
                &cfg,
                5,
                ble_task_ctx.stack_buf,
                ble_task_ctx.tcb_buf,
                0);
        }
        else
        {
            ESP_LOGE("MAIN", "Error: No hay memoria INTERNA para la tarea BLE");
            if (ble_task_ctx.stack_buf)
                heap_caps_free(ble_task_ctx.stack_buf);
            if (ble_task_ctx.tcb_buf)
                heap_caps_free(ble_task_ctx.tcb_buf);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
        xEventGroupWaitBits(init_events, EVT_BT_INIT_DONE, pdTRUE, pdFALSE, portMAX_DELAY);

        if (ble_task_ctx.handle != NULL)
        {
            vTaskDelete(ble_task_ctx.handle);
        }
        heap_caps_free(ble_task_ctx.stack_buf);
        heap_caps_free(ble_task_ctx.tcb_buf);

        ble_task_ctx.handle = NULL;
        ble_task_ctx.stack_buf = NULL;
        ble_task_ctx.tcb_buf = NULL;
        ESP_LOGW("bt_mgr", "Bluetooth enabled!");
    }
    else
    {
        ESP_LOGW("bt_mgr", "Bluetooth disabled; skipping start");
    }
    mem_diag_report("AFTER-BT");

    // *** WIFI ***//
    if (appcfg_cache_get(&cfg) == ESP_OK && cfg.wifi.enabled)
    {
        wifi_cfg_valid = true;
        wifi_task_ctx.stack_buf = heap_caps_malloc(16384, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        wifi_task_ctx.tcb_buf = heap_caps_malloc(sizeof(StaticTask_t), MALLOC_CAP_INTERNAL);
        assert(wifi_task_ctx.stack_buf && wifi_task_ctx.tcb_buf);
        wifi_task_ctx.handle = xTaskCreateStaticPinnedToCore(
            wifi_init_task,
            "wifi_init",
            MY_TASK_STACK_WORDS,
            &cfg,
            PRIO_INIT_TASK,
            wifi_task_ctx.stack_buf,
            wifi_task_ctx.tcb_buf,
            0);
        if (wifi_task_ctx.handle == NULL)
        {
            ESP_LOGE("wifi_init", "No se pudo crear screen_init task");
        }
        else
        {
            ESP_LOGI("wifi_init", "wifi_init task creada");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
        xEventGroupWaitBits(init_events, EVT_WIFI_INIT_DONE,
                            pdTRUE,
                            pdFALSE,
                            portMAX_DELAY);
        vTaskDelete(wifi_task_ctx.handle);
        heap_caps_free(wifi_task_ctx.stack_buf);
        heap_caps_free(wifi_task_ctx.tcb_buf);
        wifi_task_ctx.handle = NULL;
        wifi_task_ctx.stack_buf = NULL;
        wifi_task_ctx.tcb_buf = NULL;
        ESP_LOGW("wifi_mgr", "Wi-Fi enabled!");
    }
    else
    {
        ESP_LOGW("wifi_mgr", "Wi-Fi disabled; skipping start");
    }
    mem_diag_report("AFTER-WIFI");

    // *** ETHERNET ***//
    if (appcfg_cache_get(&cfg) == ESP_OK && cfg.eth.enabled)
    {
        eth_task_ctx.stack_buf = heap_caps_malloc(16384, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        eth_task_ctx.tcb_buf = heap_caps_malloc(sizeof(StaticTask_t), MALLOC_CAP_INTERNAL);
        assert(eth_task_ctx.stack_buf && eth_task_ctx.tcb_buf);
        eth_task_ctx.handle = xTaskCreateStaticPinnedToCore(
            eth_init_task,
            "eth_init",
            MY_TASK_STACK_WORDS,
            &cfg,
            PRIO_INIT_TASK,
            eth_task_ctx.stack_buf,
            eth_task_ctx.tcb_buf,
            0);
        if (eth_task_ctx.handle == NULL)
        {
            ESP_LOGE("eth_init", "No se pudo crear eth_init task");
        }
        else
        {
            ESP_LOGI("eth_init", "eth_init task creada");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
        xEventGroupWaitBits(init_events, EVT_ETH_INIT_DONE,
                            pdTRUE,
                            pdFALSE,
                            portMAX_DELAY);
        vTaskDelete(eth_task_ctx.handle);
        heap_caps_free(eth_task_ctx.stack_buf);
        heap_caps_free(eth_task_ctx.tcb_buf);
        eth_task_ctx.handle = NULL;
        eth_task_ctx.stack_buf = NULL;
        eth_task_ctx.tcb_buf = NULL;
        ESP_LOGW("eth_mgr", "ETH Enabled");
    }
    else
    {
        ESP_LOGW("eth_mgr", "ETH disabled; not creating task");
    }
    mem_diag_report("AFTER-ETH");

    //== == == == INICIAR TAREA DE REFRESCO DE UI == == == ==
    TaskHandle_t ui_refresh_handle = NULL;
    BaseType_t r = xTaskCreatePinnedToCore(
        ui_refresh_task,
        "ui_refresh",
        8192, // 2048 palabras (~8 KB), más que suficiente para 4 labels
        &cfg,
        PRIO_UI_LOGIC,
        &ui_refresh_handle,
        1);

    if (r != pdPASS || ui_refresh_handle == NULL)
    {
        ESP_LOGE("ui_refresh", "No se pudo crear ui_refresh_task (err=%ld)", (long)r);
    }
    else
    {
        ESP_LOGI("ui_refresh", "ui_refresh_task creada OK");
    }
    mem_diag_report("AFTER-UI-REFRESH");
    mem_diag_report_full("END");

    // Diagnostico de heap cada 30 s
    mem_diag_start_periodic(30000);

    // Traza de tareas: muestrea CPU%+estado cada 2 s.
    // Captura con: idf.py -p COMX monitor | tee captura.log
    // Analiza con: python tools/trace_analyze.py captura.log
    task_tracer_start(2000);

    while (true)
        vTaskDelay(pdMS_TO_TICKS(50));
}

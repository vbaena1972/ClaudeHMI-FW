#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "lwip/ip4_addr.h"
#include "lwip/inet.h"

#include "storage.h"
#include "wifi_mgr.h"
#include "mem_diag.h"
#include "http_api.h"
#include "eth_mgr.h"

static const char *TAG = "wifi_mgr";
static const char *TAGW = "wifi_mgr_defer";

#define WIFI_APPLY_DELAY_MS 2000 // delay para no cortar HTTP

static esp_netif_t *s_wifi_netif = NULL;
static wifi_status_cb_t s_cb = NULL;

// Config actual aplicada y config pendiente
typedef struct
{
    bool dhcp;
    char ssid[17], password[33];
    char ip[16], netmask[16], gw[16], dns1[16], dns2[16];
} wifi_view_t;

static wifi_view_t s_cfg_cur = {0};
static wifi_view_t s_cfg_pending = {0};
static bool s_have_pending = false;

static bool s_enabled = false; // estado STA activa
static bool s_allow_reconnect = true;
static TimerHandle_t s_wifi_apply_timer = NULL;

extern void ui_statusbar_request_refresh(void);

// ------------------------ Helpers / Notify -----------------------------------
static void notify(wifi_status_t st)
{
    if (s_cb)
    {
        wifi_ap_record_t ap = {0};
        int rssi = 0;
        if (st == WIFI_STATUS_CONNECTED && esp_wifi_sta_get_ap_info(&ap) == ESP_OK)
        {
            rssi = ap.rssi; // dBm
        }
        s_cb(st, rssi);
    }
}

// Compara campos relevantes de red (SSID/clave, DHCP vs estático, IP/DNS)
static bool net_cfg_equals(const wifi_view_t *a, const wifi_view_t *b)
{
#define EQS(x) (strncmp((a->x), (b->x), sizeof(a->x)) == 0)
    if (a->dhcp != b->dhcp)
        return false;
    if (!EQS(ssid))
        return false;
    if (!EQS(password))
        return false;

    // Para IP estática, comparamos los campos; si ambos son DHCP, estos valores no afectan
    if (!EQS(ip))
        return false;
    if (!EQS(netmask))
        return false;
    if (!EQS(gw))
        return false;
    if (!EQS(dns1))
        return false;
    if (!EQS(dns2))
        return false;
#undef EQS
    return true;
}

// Aplica modo IP en s_wifi_netif (DHCP o estático) usando SOLO 'dhcp'
static esp_err_t apply_ip_mode(const wifi_view_t *c)
{
    if (!s_wifi_netif)
        return ESP_ERR_INVALID_STATE;

    if (c->dhcp)
    {
        ESP_LOGI(TAG, "IP mode: DHCP");
        esp_netif_dhcpc_stop(s_wifi_netif); // ignora si ya está parado
        esp_err_t r = esp_netif_dhcpc_start(s_wifi_netif);
        if (r != ESP_OK && r != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED)
        {
            ESP_LOGE(TAG, "dhcpc_start: %s", esp_err_to_name(r));
            return r;
        }
        // limpiar DNS manual para que prevalezca DHCP
        esp_netif_dns_info_t nul = {0};
        esp_netif_set_dns_info(s_wifi_netif, ESP_NETIF_DNS_MAIN, &nul);
        esp_netif_set_dns_info(s_wifi_netif, ESP_NETIF_DNS_BACKUP, &nul);
        return ESP_OK;
    }

    ESP_LOGI(TAG, "IP mode: STATIC (%s/%s gw:%s)", c->ip, c->netmask, c->gw);

    ip4_addr_t ip = {0}, gw = {0}, mask = {0};
    if (!ip4addr_aton(c->ip, &ip) ||
        !ip4addr_aton(c->gw, &gw) ||
        !ip4addr_aton(c->netmask, &mask))
    {
        ESP_LOGE(TAG, "IP parse error");
        return ESP_ERR_INVALID_ARG;
    }

    esp_netif_ip_info_t ipi = {0};
    ipi.ip.addr = ip.addr;
    ipi.gw.addr = gw.addr;
    ipi.netmask.addr = mask.addr;

    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(s_wifi_netif));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(s_wifi_netif, &ipi));

    // DNS1 obligatorio si lo pasaste
    esp_netif_dns_info_t dns = {0};
    if (c->dns1[0] != '\0' && ip4addr_aton(c->dns1, &ip))
    {
        dns.ip.type = ESP_IPADDR_TYPE_V4;
        dns.ip.u_addr.ip4.addr = ip.addr;
        ESP_ERROR_CHECK(esp_netif_set_dns_info(s_wifi_netif, ESP_NETIF_DNS_MAIN, &dns));
    }

    // DNS2 opcional
    if (c->dns2[0] != '\0' && ip4addr_aton(c->dns2, &ip))
    {
        dns.ip.u_addr.ip4.addr = ip.addr;
        ESP_ERROR_CHECK(esp_netif_set_dns_info(s_wifi_netif, ESP_NETIF_DNS_BACKUP, &dns));
    }

    return ESP_OK;
}

// Helper para loguear DNS actuales
static void log_dns_info(void)
{
    esp_netif_dns_info_t d0 = {0}, d1 = {0};
    if (esp_netif_get_dns_info(s_wifi_netif, ESP_NETIF_DNS_MAIN, &d0) == ESP_OK)
    {
        ESP_LOGI(TAG, "DNS1: " IPSTR, IP2STR(&d0.ip.u_addr.ip4));
    }
    if (esp_netif_get_dns_info(s_wifi_netif, ESP_NETIF_DNS_BACKUP, &d1) == ESP_OK)
    {
        ESP_LOGI(TAG, "DNS2: " IPSTR, IP2STR(&d1.ip.u_addr.ip4));
    }
}

// ----------------------- Event handler --------------------------------------
static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT)
    {
        switch (id)
        {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "STA_START -> connect");
            notify(WIFI_STATUS_CONNECTING);
            ui_statusbar_request_refresh();
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGW(TAG, "STA_DISCONNECTED");
            notify(WIFI_STATUS_DISCONNECTED);
            ui_statusbar_request_refresh();

            if (s_allow_reconnect)
            {
                ESP_LOGW(TAG, "STA_DISCONNECTED (reintentando)");
                esp_wifi_connect();
            }
            else
            {
                ESP_LOGI(TAG, "STA_DISCONNECTED (no reintento; WiFi forzada OFF por Ethernet)");
            }
            break;
        default:
            break;
        }
    }
    else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        const esp_netif_ip_info_t *ipi = &ev->ip_info;
        ESP_LOGI(TAG, "GOT_IP  IP: " IPSTR "  MASK: " IPSTR "  GW: " IPSTR,
                 IP2STR(&ipi->ip), IP2STR(&ipi->netmask), IP2STR(&ipi->gw));
        log_dns_info();
        notify(WIFI_STATUS_CONNECTED);
        ui_statusbar_request_refresh();
        // --- TELEMETRÍA + HTTPD ---
        if (!eth_mgr_is_up())
        {
            mem_diag_report("WIFI:got_ip_before_http_start");
            esp_err_t r = http_api_start(); // idempotente
            ESP_LOGI(TAG, "http_api_start -> %s", esp_err_to_name(r));
            mem_diag_report("WIFI:after_http_start");
        }
        else
        {
            ESP_LOGI(TAG, "ETH is up, skip HTTPD on Wi-Fi");
        }
    }
}

// ------------------------ API pública ----------------------------------------
esp_err_t wifi_mgr_get_netinfo(wifi_netinfo_t *o)
{
    if (!o || !s_wifi_netif)
        return ESP_ERR_INVALID_ARG;
    memset(o, 0, sizeof(*o));

    // SSID/RSSI si hay enlace
    wifi_ap_record_t ap = {0};
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK)
    {
        o->connected = true;
        strncpy(o->ssid, (const char *)ap.ssid, sizeof(o->ssid) - 1);
        o->rssi_dbm = ap.rssi;
    }

    // IP info
    esp_netif_ip_info_t ipi = {0};
    if (esp_netif_get_ip_info(s_wifi_netif, &ipi) == ESP_OK)
    {
        snprintf(o->ip, sizeof(o->ip), IPSTR, IP2STR(&ipi.ip));
        snprintf(o->mask, sizeof(o->mask), IPSTR, IP2STR(&ipi.netmask));
        snprintf(o->gw, sizeof(o->gw), IPSTR, IP2STR(&ipi.gw));
    }

    // DNS
    esp_netif_dns_info_t d0 = {0}, d1 = {0};
    if (esp_netif_get_dns_info(s_wifi_netif, ESP_NETIF_DNS_MAIN, &d0) == ESP_OK)
    {
        snprintf(o->dns1, sizeof(o->dns1), IPSTR, IP2STR(&d0.ip.u_addr.ip4));
    }
    if (esp_netif_get_dns_info(s_wifi_netif, ESP_NETIF_DNS_BACKUP, &d1) == ESP_OK)
    {
        snprintf(o->dns2, sizeof(o->dns2), IPSTR, IP2STR(&d1.ip.u_addr.ip4));
    }
    return ESP_OK;
}

esp_err_t wifi_mgr_init(void)
{
    static bool inited = false;
    if (inited)
        return ESP_OK;

    // 1) esp_netif_init con tolerancia a re-init
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
        return err;

    // 2) event loop default, idem
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
        ESP_ERROR_CHECK(err);

    s_wifi_netif = esp_netif_create_default_wifi_sta();
    if (!s_wifi_netif)
        return ESP_ERR_NO_MEM;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_LOGI("Wifi_Status", "bien aqui 1");
    err = esp_wifi_init(&cfg);
    if (err == ESP_ERR_INVALID_STATE)
    {
        ESP_LOGW(TAG, "Wifi controller already init");
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Wifi controller init failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI("Wifi_Status", "bien aqui 2");
    err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);

    if (err == ESP_ERR_INVALID_STATE)
    {
        ESP_LOGW(TAG, "Wifi ID event already init");
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Wifi ID event init failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI("Wifi_Status", "bien aqui 3");

    err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    if (err == ESP_ERR_INVALID_STATE)
    {
        ESP_LOGW(TAG, "Wifi IP event already init");
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Wifi IP event init failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI("Wifi_Status", "bien aqui 4");
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err == ESP_ERR_INVALID_STATE)
    {
        ESP_LOGW(TAG, "Wifi Set Mode STA already init");
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Wifi Set Mode STA init failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI("Wifi_Status", "bien aqui 5");

    inited = true;
    return ESP_OK;
}

// Timer callback: aplica la config pendiente si cambió algo relevante
static void wifi_apply_cb(TimerHandle_t xTimer)
{
    (void)xTimer;
    if (!s_have_pending)
        return;

    // Snapshot y limpia bandera de pending
    s_have_pending = false;

    wifi_view_t next = s_cfg_pending;

    // Si no cambió nada relevante, no hagas nada
    if (net_cfg_equals(&s_cfg_cur, &next))
    {
        ESP_LOGI(TAGW, "No net changes; nothing to apply");
        return;
    }

    ESP_LOGI(TAGW, "Applying Wi-Fi config (deferred)");
    // Construye config STA
    wifi_config_t w = {0};
    strncpy((char *)w.sta.ssid, (const char *)next.ssid, sizeof(w.sta.ssid) - 1);
    strncpy((char *)w.sta.password, (const char *)next.password, sizeof(w.sta.password) - 1);
    w.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
#if defined(WPA3_SAE_PWE_BOTH)
    w.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
#elif defined(WPA3_SAE_PWE_H2E_MIXED)
    w.sta.sae_pwe_h2e = WPA3_SAE_PWE_H2E_MIXED;
#elif defined(WPA3_SAE_PWE_H2E)
    w.sta.sae_pwe_h2e = WPA3_SAE_PWE_H2E;
#else
    w.sta.sae_pwe_h2e = WPA3_SAE_PWE_UNSPECIFIED;
#endif

    // Si la STA está encendida, desconecta para aplicar SSID/clave/IP
    if (s_enabled)
    {
        esp_wifi_disconnect();
    }

    wifi_mode_t mode;
    esp_err_t err = esp_wifi_get_mode(&mode);
    if (err == ESP_ERR_WIFI_NOT_INIT)
    {
        // WiFi nunca inicializó o se cayó: intentar init otra vez
        if (wifi_mgr_init() != ESP_OK)
        {
            ESP_LOGE(TAGW, "wifi_apply_cb: wifi_mgr_init() failed, aborting apply");
            return;
        }
        // opcional: esp_wifi_set_mode(WIFI_MODE_STA); etc.
    }

    // Aplica configuración Wi-Fi y modo IP
    err = esp_wifi_set_config(WIFI_IF_STA, &w);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAGW, "esp_wifi_set_config: %s", esp_err_to_name(err));
        // aún así actualiza s_cfg_cur para evitar loops
    }
    (void)apply_ip_mode(&next);

    // Reconecta si está habilitada la STA
    if (s_enabled)
    {
        esp_wifi_connect();
    }

    // Actualiza config actual
    s_cfg_cur = next;

    ui_statusbar_request_refresh();
}

// Programa aplicación diferida (coalesce si ya hay una pendiente)
static void wifi_schedule_apply(void)
{
    if (!s_wifi_apply_timer)
    {
        s_wifi_apply_timer = xTimerCreate("wifi_apply",
                                          pdMS_TO_TICKS(WIFI_APPLY_DELAY_MS),
                                          pdFALSE,
                                          NULL,
                                          wifi_apply_cb);
        if (!s_wifi_apply_timer)
        {
            ESP_LOGE(TAGW, "xTimerCreate failed");
            return;
        }
    }
    xTimerStop(s_wifi_apply_timer, 0);
    xTimerChangePeriod(s_wifi_apply_timer, pdMS_TO_TICKS(WIFI_APPLY_DELAY_MS), 0);
    xTimerStart(s_wifi_apply_timer, 0);
    ESP_LOGI(TAGW, "Wi-Fi apply scheduled (%d ms)", WIFI_APPLY_DELAY_MS);
}

static void wifi_view_from_appcfg(wifi_view_t *out, const AppConfig *c)
{
    memset(out, 0, sizeof(*out));
    out->dhcp = (strcasecmp(c->wifi.ip_mode, "dhcp") == 0);
    strncpy(out->ssid, c->wifi.ssid, sizeof(out->ssid) - 1);
    strncpy(out->password, c->wifi.password, sizeof(out->password) - 1);
    strncpy(out->ip, c->wifi.ip, sizeof(out->ip) - 1);
    strncpy(out->netmask, c->wifi.mask, sizeof(out->netmask) - 1);
    strncpy(out->gw, c->wifi.gw, sizeof(out->gw) - 1);
    strncpy(out->dns1, c->wifi.dns1, sizeof(out->dns1) - 1);
    strncpy(out->dns2, c->wifi.dns2, sizeof(out->dns2) - 1);
}

esp_err_t wifi_mgr_apply_from_cache(void)
{
    AppConfig cfg = {0};
    appcfg_cache_get(&cfg);

    wifi_view_t v;
    wifi_view_from_appcfg(&v, &cfg);

    if (net_cfg_equals(&s_cfg_cur, &v))
    {
        return ESP_OK;
    }

    s_cfg_pending = v;
    s_have_pending = true;

    wifi_schedule_apply();

    return ESP_OK;
}

esp_err_t wifi_mgr_start(void)
{
    s_allow_reconnect = true;
    s_enabled = true;
    notify(WIFI_STATUS_CONNECTING);
    ui_statusbar_request_refresh();
    return esp_wifi_start();
}

esp_err_t wifi_mgr_stop(void)
{
    s_allow_reconnect = false;
    s_enabled = false;
    notify(WIFI_STATUS_OFF);
    ui_statusbar_request_refresh();
    return esp_wifi_stop();
}

esp_err_t wifi_mgr_update_credentials(const char *ssid, const char *password)
{
    wifi_config_t wifi_config = {0};
    strlcpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strlcpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err == ESP_OK)
    {
        ESP_LOGI("WIFI_MGR", "Credenciales actualizadas. Reconectando...");
        esp_wifi_disconnect();
        esp_wifi_connect();
    }
    return err;
}

void wifi_mgr_set_status_cb(wifi_status_cb_t cb)
{
    s_cb = cb;

    // Snapshot inmediato del estado actual
    if (!s_wifi_netif || !s_cb)
        return;

    // ¿Estamos conectados al AP?
    wifi_ap_record_t ap = {0};
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK)
    {
        // Conectado (enlace con AP). Mirar si ya hay IP
        esp_netif_ip_info_t ipi = {0};
        if (esp_netif_get_ip_info(s_wifi_netif, &ipi) == ESP_OK && ipi.ip.addr != 0)
        {
            s_cb(WIFI_STATUS_CONNECTED, ap.rssi);
        }
        else
        {
            s_cb(WIFI_STATUS_CONNECTING, ap.rssi);
        }
        return;
    }

    // No hay enlace con AP
    if (!s_enabled)
        s_cb(WIFI_STATUS_OFF, 0);
    else
        s_cb(WIFI_STATUS_DISCONNECTED, 0);
}

bool wifi_mgr_is_enabled(void)
{
    return s_enabled;
}

bool wifi_mgr_has_ip(void)
{
    if (!s_wifi_netif)
        return false;
    esp_netif_ip_info_t ipi = {0};
    if (esp_netif_get_ip_info(s_wifi_netif, &ipi) != ESP_OK)
        return false;
    return ipi.ip.addr != 0;
}

esp_netif_t *wifi_mgr_get_netif(void)
{
    return s_wifi_netif;
}

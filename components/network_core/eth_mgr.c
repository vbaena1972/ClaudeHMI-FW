#include <string.h>
#include <stdlib.h>

#include "eth_mgr.h"
#include "ethernet_init.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_eth_netif_glue.h"
#include "driver/gpio.h"

#include "lwip/inet.h"
#include "lwip/ip4_addr.h"

#include "state_pub.h"
#include "storage.h"
#include "wifi_mgr.h"
#include "ui_statusbar_controller.h"
#include "mem_diag.h"
#include "http_api.h"

static const char *TAG = "eth_mgr";

static bool s_link_up = false;

typedef struct
{
    bool dhcp;
    char hostname[33];
    char ip[16], mask[16], gw[16], dns1[16], dns2[16];
} eth_view_t;

static eth_view_t s_cfg = {0};

// Manejamos múltiples puertos si example_eth_init() devuelve varios handles
static esp_eth_handle_t *s_eth_handles = NULL;
static uint8_t s_eth_cnt = 0;
static esp_netif_t **s_eth_netif = NULL;
static bool s_initialized = false;

// -------------------- Eventos --------------------
static void on_eth_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    switch (id)
    {
    case ETHERNET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "ETH link up");
        s_link_up = true;

        // Ethernet activo => WiFi debe apagarse
        wifi_mgr_stop();

        // La red está (o estará) disponible por cable
        state_on_net_available(true);

        // Actualiza barra de estado
        ui_statusbar_request_refresh();
        ESP_LOGI("MEMCHK", "Tasks=%u", uxTaskGetNumberOfTasks());
        mem_diag_report("RUNTIME-AFTER-ETH-UP_WIFI-DOWN");
        ESP_LOGI("MEMCHK",
                 "netif_wifi=%p netif_eth=%p",
                 (void *)wifi_mgr_get_netif(),
                 (s_eth_netif && s_eth_cnt > 0) ? (void *)s_eth_netif[0] : NULL);

        break;

    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "ETH link down");
        s_link_up = false;
        mem_diag_report("ETH_DOWN:before_http_stop");

        http_api_stop(); // importante: bajar HTTPD primero

        mem_diag_report("ETH_DOWN:after_http_stop");
        // Intenta levantar WiFi de nuevo.
        wifi_mgr_start();

        // Publica que por ETH ya no tenemos red. WiFi la volverá a publicar si obtiene IP.
        state_on_net_available(false);

        // Actualiza barra de estado
        ui_statusbar_request_refresh();
        mem_diag_report("RUNTIME-AFTER-ETH-DOWN_WIFI-UP");
        break;

    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "ETH started");
        break;

    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "ETH stopped");
        s_link_up = false;

        // Levanta WiFi cuando ETH fue detenida del todo
        wifi_mgr_start();

        state_on_net_available(false);
        ui_statusbar_request_refresh();
        break;

    default:
        break;
    }
}

static void on_ip_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (id == IP_EVENT_ETH_GOT_IP)
    {
        ip_event_got_ip_t *e = (ip_event_got_ip_t *)data;

        ESP_LOGI(TAG, "ETH got IP: " IPSTR, IP2STR(&e->ip_info.ip));
        state_on_net_available(true);

        // <<< CAMBIO: refresco barra cuando realmente ya tenemos IP en ETH
        ui_statusbar_request_refresh();

        // ... resto de tu código que persiste IP/DNS a NVS ...
    }
}
// -------------------- Config IP --------------------
static void apply_ip_config(esp_netif_t *netif)
{
    if (!netif)
        return;

    if (s_cfg.dhcp)
    {
        // DHCP
        esp_netif_dhcpc_start(netif);
        return;
    }

    // Estática
    esp_netif_dhcpc_stop(netif);

    esp_netif_ip_info_t ipi = {0};
    bool ok = true;

    ip4_addr_t ip_tmp, mask_tmp, gw_tmp;

    if (!(s_cfg.ip[0] && ip4addr_aton(s_cfg.ip, &ip_tmp)))
        ok = false;
    if (!(s_cfg.mask[0] && ip4addr_aton(s_cfg.mask, &mask_tmp)))
        ok = false;
    if (!(s_cfg.gw[0] && ip4addr_aton(s_cfg.gw, &gw_tmp)))
        ok = false;

    if (ok)
    {
        ipi.ip.addr = ip_tmp.addr;
        ipi.netmask.addr = mask_tmp.addr;
        ipi.gw.addr = gw_tmp.addr;
        ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &ipi));

        // DNS opcional
        esp_netif_dns_info_t di = {0};
        ip4_addr_t d1, d2;

        if (s_cfg.dns1[0] && ip4addr_aton(s_cfg.dns1, &d1))
        {
            di.ip.type = ESP_IPADDR_TYPE_V4;
            di.ip.u_addr.ip4.addr = d1.addr;
            ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &di));
        }
        if (s_cfg.dns2[0] && ip4addr_aton(s_cfg.dns2, &d2))
        {
            di.ip.type = ESP_IPADDR_TYPE_V4;
            di.ip.u_addr.ip4.addr = d2.addr;
            ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, ESP_NETIF_DNS_BACKUP, &di));
        }
    }
    else
    {
        ESP_LOGW(TAG, "IP estática inválida; usando DHCP");
        esp_netif_dhcpc_start(netif);
    }
}

// -------------------- API pública --------------------
static void appcfg_to_eth_view(const AppConfig *c, eth_view_t *out)
{
    memset(out, 0, sizeof(*out));

    out->dhcp = (strcmp(c->eth.ip_mode, "dhcp") == 0);

    strncpy(out->hostname, c->eth.hostname, sizeof(out->hostname) - 1);
    out->hostname[sizeof(out->hostname) - 1] = '\0';
    strncpy(out->ip, c->eth.ip, sizeof(out->ip) - 1);
    out->ip[sizeof(out->ip) - 1] = '\0';
    strncpy(out->mask, c->eth.mask, sizeof(out->mask) - 1);
    out->mask[sizeof(out->mask) - 1] = '\0';
    strncpy(out->gw, c->eth.gw, sizeof(out->gw) - 1);
    out->gw[sizeof(out->gw) - 1] = '\0';
    strncpy(out->dns1, c->eth.dns1, sizeof(out->dns1) - 1);
    out->dns1[sizeof(out->dns1) - 1] = '\0';
    strncpy(out->dns2, c->eth.dns2, sizeof(out->dns2) - 1);
    out->dns2[sizeof(out->dns2) - 1] = '\0';
}

// Conveniencia: inicializa ETH leyendo de NVS via storage.c
esp_err_t eth_mgr_init_from_storage(void)
{
    AppConfig cfg;
    if (appcfg_cache_get(&cfg) != ESP_OK)
        return ESP_FAIL;
    // Poblar la vista local global s_cfg
    appcfg_to_eth_view(&cfg, &s_cfg);
    // Iniciar con la firma actual (sin argumentos)
    return eth_mgr_init();
}

static void eth_view_from_appcfg(eth_view_t *out, const AppConfig *c)
{
    memset(out, 0, sizeof(*out));
    out->dhcp = (strcasecmp(c->eth.ip_mode, "dhcp") == 0);
    strncpy(out->hostname, c->eth.hostname, sizeof(out->hostname) - 1);
    strncpy(out->ip, c->eth.ip, sizeof(out->ip) - 1);
    strncpy(out->mask, c->eth.mask, sizeof(out->mask) - 1);
    strncpy(out->gw, c->eth.gw, sizeof(out->gw) - 1);
    strncpy(out->dns1, c->eth.dns1, sizeof(out->dns1) - 1);
    strncpy(out->dns2, c->eth.dns2, sizeof(out->dns2) - 1);
}

esp_err_t eth_mgr_init(void)
{
    AppConfig cfg = {0};
    appcfg_cache_get(&cfg);
    eth_view_from_appcfg(&s_cfg, &cfg);

    // si ya está inicializado, NO volvemos a crear drivers ni netifs.
    if (s_initialized)
    {
        // re-aplicar hostname e IP/DHCP a las netifs existentes
        for (uint8_t i = 0; i < s_eth_cnt; ++i)
        {
            if (s_cfg.hostname[0])
            {
                (void)esp_netif_set_hostname(s_eth_netif[i], s_cfg.hostname);
            }
            apply_ip_config(s_eth_netif[i]); // esto ya sabe si es DHCP o estática
        }
        return ESP_OK;
    }

    esp_err_t r;
    r = esp_netif_init();
    if (r != ESP_OK && r != ESP_ERR_INVALID_STATE)
        return r;

    r = esp_event_loop_create_default();
    if (r != ESP_OK && r != ESP_ERR_INVALID_STATE)
        return r;

    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &on_eth_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &on_ip_event, NULL));

    ESP_ERROR_CHECK(example_eth_init(&s_eth_handles, &s_eth_cnt));
    if (s_eth_cnt == 0 || s_eth_handles == NULL)
    {
        ESP_LOGE(TAG, "No Ethernet drivers initialized");
        return ESP_FAIL;
    }

    s_eth_netif = (esp_netif_t **)calloc(s_eth_cnt, sizeof(esp_netif_t *));
    if (!s_eth_netif)
        return ESP_ERR_NO_MEM;

    for (uint8_t i = 0; i < s_eth_cnt; ++i)
    {
        esp_netif_config_t cfg_netif = ESP_NETIF_DEFAULT_ETH();
        s_eth_netif[i] = esp_netif_new(&cfg_netif);
        if (!s_eth_netif[i])
        {
            ESP_LOGE(TAG, "esp_netif_new failed");
            return ESP_FAIL;
        }
        ESP_ERROR_CHECK(esp_netif_attach(s_eth_netif[i],
                                         esp_eth_new_netif_glue(s_eth_handles[i])));

        if (s_cfg.hostname[0])
        {
            (void)esp_netif_set_hostname(s_eth_netif[i], s_cfg.hostname);
        }

        apply_ip_config(s_eth_netif[i]);
    }

    s_initialized = true;
    return ESP_OK;
}

esp_err_t eth_mgr_start(void)
{
    if (!s_eth_handles || s_eth_cnt == 0)
        return ESP_ERR_INVALID_STATE;

    for (uint8_t i = 0; i < s_eth_cnt; ++i)
    {
        esp_err_t r = esp_eth_start(s_eth_handles[i]);
        if (r == ESP_ERR_INVALID_STATE)
        {
            ESP_LOGW("eth_mgr", "eth %u ya estaba iniciada", (unsigned)i);
            r = ESP_OK;
        }
        if (r != ESP_OK)
        {
            ESP_LOGE("eth_mgr", "falló esp_eth_start(%u): %d", (unsigned)i, r);
            return r;
        }
    }

    ESP_LOGI("eth_mgr", "Ethernet started (%u)", (unsigned)s_eth_cnt);
    return ESP_OK;
}

esp_err_t eth_mgr_stop(void)
{
    if (!s_eth_handles || s_eth_cnt == 0)
        return ESP_ERR_INVALID_STATE;
    for (uint8_t i = 0; i < s_eth_cnt; ++i)
    {
        (void)esp_eth_stop(s_eth_handles[i]);
    }
    s_link_up = false;
    return ESP_OK;
}

bool eth_mgr_is_up(void)
{
    return s_link_up;
}

bool eth_mgr_link_up(void)
{
    return s_link_up;
}
#include "cert_store.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "psa/crypto.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static const char *TAG = "cert_store";
#define NVS_NS "certs"

static const char *key_for_kind(cert_kind_t k)
{
    switch (k)
    {
    case CERT_SERVER_CA:
        return "server_ca";
    case CERT_CLIENT_CERT:
        return "client_crt";
    case CERT_CLIENT_KEY:
        return "client_key";
    default:
        return "";
    }
}

static void sha256_hex(const uint8_t *buf, size_t len, char out_hex[65])
{
    unsigned char sum[32];
    size_t out_len = 0;

    // Nueva API estándar PSA de MbedTLS 4.0 para ESP-IDF v6.x
    psa_hash_compute(
        PSA_ALG_SHA_256, // Algoritmo seleccionado
        buf,             // Buffer de entrada
        len,             // Longitud de los datos
        sum,             // Buffer de salida para el hash
        sizeof(sum),     // Tamaño máximo asignado para la salida
        &out_len         // Variable que guardará la longitud real procesada
    );

    // Tu lógica de conversión a caracteres hexadecimales se mantiene idéntica
    for (int i = 0; i < 32; i++)
    {
        sprintf(&out_hex[i * 2], "%02x", sum[i]);
    }
    out_hex[64] = '\0';
}

esp_err_t cert_store_write(cert_kind_t kind, const void *data, size_t len)
{
    if (!data || !len)
        return ESP_ERR_INVALID_ARG;
    const char *key = key_for_kind(kind);
    if (!key[0])
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t h;
    esp_err_t r = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (r != ESP_OK)
        return r;

    r = nvs_set_blob(h, key, data, len);
    if (r == ESP_OK)
        r = nvs_commit(h);
    nvs_close(h);

    if (r == ESP_OK)
    {
        char hex[65];
        sha256_hex((const uint8_t *)data, len, hex);
        ESP_LOGI(TAG, "WROTE %s len=%u sha256=%s", key, (unsigned)len, hex);
    }
    return r;
}

esp_err_t cert_store_load(cert_kind_t kind, uint8_t **out_buf, size_t *out_len)
{
    if (!out_buf || !out_len)
        return ESP_ERR_INVALID_ARG;
    *out_buf = NULL;
    *out_len = 0;
    const char *key = key_for_kind(kind);
    if (!key[0])
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t h;
    esp_err_t r = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (r != ESP_OK)
        return r;

    size_t len = 0;
    r = nvs_get_blob(h, key, NULL, &len);
    if (r != ESP_OK)
    {
        nvs_close(h);
        return r;
    }

    uint8_t *buf = (uint8_t *)malloc(len);
    if (!buf)
    {
        nvs_close(h);
        return ESP_ERR_NO_MEM;
    }

    r = nvs_get_blob(h, key, buf, &len);
    nvs_close(h);
    if (r != ESP_OK)
    {
        free(buf);
        return r;
    }

    *out_buf = buf;
    *out_len = len;
    return ESP_OK;
}

esp_err_t cert_store_erase(cert_kind_t kind)
{
    const char *key = key_for_kind(kind);
    if (!key[0])
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t h;
    esp_err_t r = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (r != ESP_OK)
        return r;

    r = nvs_erase_key(h, key);
    if (r == ESP_ERR_NVS_NOT_FOUND)
        r = ESP_OK;
    if (r == ESP_OK)
        r = nvs_commit(h);
    nvs_close(h);
    ESP_LOGI(TAG, "ERASE %s -> %s", key, esp_err_to_name(r));
    return r;
}

esp_err_t cert_store_info(cert_kind_t kind, cert_info_t *info)
{
    if (!info)
        return ESP_ERR_INVALID_ARG;
    memset(info, 0, sizeof(*info));
    const char *key = key_for_kind(kind);
    if (!key[0])
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t h;
    esp_err_t r = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (r != ESP_OK)
        return r;

    size_t len = 0;
    r = nvs_get_blob(h, key, NULL, &len);
    if (r == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_close(h);
        info->present = false;
        return ESP_OK;
    }
    if (r != ESP_OK)
    {
        nvs_close(h);
        return r;
    }

    uint8_t *buf = (uint8_t *)malloc(len);
    if (!buf)
    {
        nvs_close(h);
        return ESP_ERR_NO_MEM;
    }

    r = nvs_get_blob(h, key, buf, &len);
    nvs_close(h);
    if (r != ESP_OK)
    {
        free(buf);
        return r;
    }

    info->present = true;
    info->length = len;
    sha256_hex(buf, len, info->sha256_hex);
    free(buf);
    return ESP_OK;
}

esp_err_t cert_store_erase_all(void)
{
    esp_err_t r = cert_store_erase(CERT_SERVER_CA);
    if (r != ESP_OK)
        return r;
    r = cert_store_erase(CERT_CLIENT_CERT);
    if (r != ESP_OK)
        return r;
    r = cert_store_erase(CERT_CLIENT_KEY);
    return r;
}

// --------- Importar desde SD ---------

static esp_err_t read_file_to_buf(const char *path, uint8_t **out, size_t *olen)
{
    *out = NULL;
    *olen = 0;
    FILE *f = fopen(path, "rb");
    if (!f)
        return ESP_ERR_NOT_FOUND;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz <= 0)
    {
        fclose(f);
        return ESP_ERR_INVALID_SIZE;
    }
    fseek(f, 0, SEEK_SET);

    // 1. Asignamos sz + 1 para reservar espacio para el terminador nulo
    uint8_t *buf = (uint8_t *)malloc(sz + 1);
    if (!buf)
    {
        fclose(f);
        return ESP_ERR_NO_MEM;
    }
    size_t rd = fread(buf, 1, sz, f);
    fclose(f);
    if (rd != (size_t)sz)
    {
        free(buf);
        return ESP_ERR_INVALID_SIZE;
    }

    // 2. INYECCIÓN CRÍTICA: Forzamos el fin de cadena de texto en formato C
    buf[sz] = '\0';

    *out = buf;

    *olen = rd;
    return ESP_OK;
}

esp_err_t cert_store_import_from_sd(const char *dir_path)
{
    char p_ca[128], p_crt[128], p_key[128];
    // ¡AQUÍ ESTÁ EL CAMBIO DE NOMBRES!
    snprintf(p_ca, sizeof(p_ca), "%s/AmazonRootCA1.pem", dir_path);
    snprintf(p_crt, sizeof(p_crt), "%s/certificate.pem.crt", dir_path);
    snprintf(p_key, sizeof(p_key), "%s/private.pem.key", dir_path);

    uint8_t *buf = NULL;
    size_t len = 0;
    esp_err_t r, overall = ESP_OK;

    // CA
    r = read_file_to_buf(p_ca, &buf, &len);
    if (r == ESP_OK)
    {
        overall |= cert_store_write(CERT_SERVER_CA, buf, len);
        free(buf);
    }
    // CRT
    r = read_file_to_buf(p_crt, &buf, &len);
    if (r == ESP_OK)
    {
        overall |= cert_store_write(CERT_CLIENT_CERT, buf, len);
        free(buf);
    }
    // KEY
    r = read_file_to_buf(p_key, &buf, &len);
    if (r == ESP_OK)
    {
        overall |= cert_store_write(CERT_CLIENT_KEY, buf, len);
        free(buf);
    }

    return overall;
}

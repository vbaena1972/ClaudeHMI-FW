#pragma once
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CERT_SERVER_CA = 0,
    CERT_CLIENT_CERT,
    CERT_CLIENT_KEY,
} cert_kind_t;

typedef struct {
    bool   present;
    size_t length;
    char   sha256_hex[65];   // opcional; "" si no calculado
} cert_info_t;

// Guarda un certificado (PEM o DER) en NVS (namespace "certs")
esp_err_t cert_store_write(cert_kind_t kind, const void *data, size_t len);

// Lee un certificado a HEAP (malloc); debes free(*out_buf)
esp_err_t cert_store_load(cert_kind_t kind, uint8_t **out_buf, size_t *out_len);

// Borra un certificado
esp_err_t cert_store_erase(cert_kind_t kind);

// Info (si existe, tamaño y sha256)
esp_err_t cert_store_info(cert_kind_t kind, cert_info_t *info);

// Importar desde microSD. Busca fijos por defecto:
//   server_ca.pem, client_cert.pem, client_key.pem en dir_path
// Si un archivo no existe, lo omite (no es error).
esp_err_t cert_store_import_from_sd(const char *dir_path);

// Elimina todos (útil para UI “Borrar certs”)
esp_err_t cert_store_erase_all(void);

#ifdef __cplusplus
}
#endif

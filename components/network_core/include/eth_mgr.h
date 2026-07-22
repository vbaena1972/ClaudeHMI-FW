#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include "storage.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t eth_mgr_init(void);
esp_err_t eth_mgr_start(void);
esp_err_t eth_mgr_stop(void);
bool      eth_mgr_is_up(void);
esp_err_t eth_mgr_init_from_storage(void);

#ifdef __cplusplus
}
#endif

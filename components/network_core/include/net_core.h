#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Iniciador global del orquestador
esp_err_t net_core_init(void); 

#ifdef __cplusplus
}
#endif
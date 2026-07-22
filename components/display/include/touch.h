/* MIT License */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_lcd_touch.h"

#define BSP_I2C_NUM             1
#define BSP_I2C_CLK_SPEED_HZ    100000

/** * @brief BSP touch configuration structure * */
typedef struct {
    void *dummy;    /*!< Prepared for future use. */
} bsp_touch_config_t;

/**  * @brief Create new touchscreen *
 * @param[in]  config    touch configuration
 * @param[out] ret_touch esp_lcd_touch touchscreen handle
 * @return
 *      - ESP_OK         On success
 *      - Else           esp_lcd_touch failure
 */
esp_err_t bsp_touch_new(const bsp_touch_config_t *config, esp_lcd_touch_handle_t *ret_touch,  i2c_master_bus_handle_t touch_handle);

#ifdef __cplusplus
}
#endif
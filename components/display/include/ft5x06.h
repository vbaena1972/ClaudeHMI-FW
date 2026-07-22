/* MIT License */

/** * @brief ESP BSP: Flow and Presure Meter (ESP32-S3) */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "display.h"
#include "touch.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch_ft5x06.h"
#include "driver/i2c_master.h"
//#include "esp_lcd_st7796.h"
//#include "bsp_err_check.h"


/**************************************************************************************************
 *  BSP Capabilities
 **************************************************************************************************/
#define BSP_LCD_TP_INT       (GPIO_NUM_39)  
#define BSP_LCD_TP_RST       (GPIO_NUM_40) 

lv_indev_t *bsp_display_indev_init(lv_display_t *disp,  i2c_master_bus_handle_t bus_handle);
lv_indev_t *bsp_display_get_input_dev(void);

#ifdef __cplusplus
}
#endif


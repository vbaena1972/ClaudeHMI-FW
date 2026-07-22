/* MIT License */
/** * @brief ESP BSP: Flow and Presure Meter (ESP32-S3) */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include "display.h"

#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_st7796.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

/**************************************************************************************************
 *  BSP Capabilities
 **************************************************************************************************/
/* Display - st7796 SPI */
#define BSP_LCD_WIDTH        (8) 
#define BSP_LCD_CS           (GPIO_NUM_3) 
#define BSP_LCD_DC           (GPIO_NUM_4)  
#define BSP_LCD_SCLK         (GPIO_NUM_6) 
#define BSP_LCD_MOSI         (GPIO_NUM_7) 
#define BSP_LCD_RST          (GPIO_NUM_5)  
#define BSP_LCD_MISO         (GPIO_NUM_8) 
#define BSP_LCD_BACKLIGHT    (GPIO_NUM_2) 

#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8

/* Display Brightness */
#define LCD_LEDC_CH            1 //CONFIG_BSP_DISPLAY_BRIGHTNESS_LEDC_CH

/** * @brief BSP display configuration structure * */
typedef struct {
    lvgl_port_cfg_t lvgl_port_cfg;  /*!< LVGL port configuration */
    uint32_t        buffer_size;    /*!< Size of the buffer for the screen in pixels */
    bool            double_buffer;  /*!< True, if should be allocated two buffers */
    struct {
        unsigned int buff_dma: 1;    /*!< Allocated LVGL buffer will be DMA capable */
        unsigned int buff_spiram: 1; /*!< Allocated LVGL buffer will be in PSRAM */
    } flags;
} bsp_display_cfg_t;

static esp_err_t bsp_display_brightness_init(void);
lv_display_t *bsp_display_start(void);
bool bsp_display_lock(uint32_t timeout_ms);
void bsp_display_unlock(void);
esp_err_t bsp_display_on(void);
esp_err_t bsp_display_off(void);
esp_err_t bsp_display_brightness_set(int percent);
esp_err_t bsp_display_backlight_off(void);
esp_err_t bsp_display_backlight_on(void);
void bsp_display_rotate(lv_disp_t *disp, lv_display_rotation_t rotation);

#ifdef __cplusplus
}
#endif


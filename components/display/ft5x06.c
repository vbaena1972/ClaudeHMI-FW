/* MIT License */
#include "ft5x06.h"
#include "esp_lcd_touch_ft5x06.h"

static const char *TAG = "ft5x06:";

static lv_indev_t *disp_indev = NULL;
static esp_lcd_touch_handle_t tp; // LCD touch handle

static SemaphoreHandle_t refresh_finish = NULL;

lv_indev_t *bsp_display_indev_init(lv_display_t *disp, i2c_master_bus_handle_t bus_handle)
{

    ESP_ERROR_CHECK(bsp_touch_new(NULL, &tp, bus_handle));
    ESP_LOGI(TAG, "BIEN hasta aqui");
    assert(tp);

    /// Add touch input (for selected screen)
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = tp,
    };

    return lvgl_port_add_touch(&touch_cfg);
}

lv_indev_t *bsp_display_get_input_dev(void)
{
    return disp_indev;
}

esp_err_t bsp_touch_new(const bsp_touch_config_t *config, esp_lcd_touch_handle_t *ret_touch, i2c_master_bus_handle_t touch_handle)
{
   // ESP_ERROR_CHECK(i2c_master_get_bus_handle(BSP_I2C_NUM, &touch_handle));
    // Initialize touch
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = BSP_LCD_V_RES,
        .y_max = BSP_LCD_H_RES,
        .rst_gpio_num = BSP_LCD_TP_RST, // Shared with LCD reset
        .int_gpio_num = BSP_LCD_TP_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;

    const esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(touch_handle, &tp_io_config, &tp_io_handle));
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, ret_touch));
   // assert(tp);

    return ESP_OK;
}

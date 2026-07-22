/* MIT License */
#include "st7796.h"
#include "esp_lcd_st7796.h"

// Using SPI2 in the example
#define LCD_HOST SPI2_HOST

static const char *TAG = "st7796:";

static lv_indev_t *disp_indev = NULL;
static esp_lcd_touch_handle_t tp; // LCD touch handle
static esp_lcd_panel_handle_t panel_handle = NULL;

// static SemaphoreHandle_t refresh_finish = NULL;
static esp_err_t bsp_display_brightness_init(void)
{
    // Setup LEDC peripheral for PWM backlight control
    const ledc_channel_config_t LCD_backlight_channel = {
        .gpio_num = BSP_LCD_BACKLIGHT,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LCD_LEDC_CH,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = 1,
        .duty = 0,
        .hpoint = 0};
    const ledc_timer_config_t LCD_backlight_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = 1,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK};

    ESP_ERROR_CHECK(ledc_timer_config(&LCD_backlight_timer));
    ESP_ERROR_CHECK(ledc_channel_config(&LCD_backlight_channel));

    return ESP_OK;
}

lv_display_t *bsp_display_start(void)
{
    ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t bus_config = ST7796_PANEL_BUS_SPI_CONFIG(BSP_LCD_SCLK, BSP_LCD_MOSI,
                                                                    BSP_LCD_H_RES * 8 * sizeof(uint16_t));
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_config, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    const esp_lcd_panel_io_spi_config_t io_config = ST7796_PANEL_IO_SPI_CONFIG(BSP_LCD_CS, BSP_LCD_DC,
                                                                               NULL, NULL);
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install ST7796 panel driver");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BSP_LCD_RST, // Set to -1 if not use
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16, // Implemented by LCD command `3Ah` (16/18)
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    // ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, true));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    /* Add LCD screen */
    static lv_display_t *disp;
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        /* 40 líneas x 320 px x 2 bytes = ~25.6 KB (en DRAM interna DMA) */
        .buffer_size = BSP_LCD_H_RES * 40 * sizeof(uint16_t),
        .double_buffer = false,
        //.double_buffer = true,
        .hres = BSP_LCD_H_RES,
        .vres = BSP_LCD_V_RES,
        .monochrome = false,
        .color_format = LV_COLOR_FORMAT_NATIVE,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            //.buff_dma = true, 
            //.buff_spiram = false, 
            .buff_dma = true, 
            .buff_spiram = true, 
            .swap_bytes = 1}};

    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_cfg.task_priority = 5;       // Prioridad ALTA (Mayor que ui_refresh=2)
    lvgl_cfg.task_stack = 7168;       // Stack seguro
    lvgl_cfg.task_affinity = 1;       // ANCLAR AL NÚCLEO 1 (Mismo que tus tareas app)
                                      // Esto evita problemas de coherencia de caché.
    esp_err_t err = lvgl_port_init(&lvgl_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al iniciar LVGL Port: %s", esp_err_to_name(err));
        return NULL;
    }
    disp = lvgl_port_add_disp(&disp_cfg);
    ESP_ERROR_CHECK(bsp_display_brightness_init());
    return disp;
}

bool bsp_display_lock(uint32_t timeout_ms)
{
    return lvgl_port_lock(timeout_ms);
}

void bsp_display_unlock(void)
{
    lvgl_port_unlock();
}

esp_err_t bsp_display_on(void)
{
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    return ESP_OK;
}

esp_err_t bsp_display_off(void)
{
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));
    return ESP_OK;
}

esp_err_t bsp_display_brightness_set(int percent)
{
    if (percent > 100)
    {
        percent = 100;
    }
    if (percent < 0)
    {
        percent = 0;
    }

    ESP_LOGI(TAG, "Setting LCD backlight: %d%%", percent);
    uint32_t duty_cycle = (1023 * percent) / 100; // LEDC resolution set to 10bits, thus: 100% = 1023
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LCD_LEDC_CH, duty_cycle));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LCD_LEDC_CH));

    return ESP_OK;
}

esp_err_t bsp_display_backlight_off(void)
{
    return bsp_display_brightness_set(0);
}

esp_err_t bsp_display_backlight_on(void)
{
    return bsp_display_brightness_set(100);
}

void bsp_display_rotate(lv_disp_t *disp, lv_display_rotation_t rotation)
{
    lv_disp_set_rotation(disp, rotation);
}
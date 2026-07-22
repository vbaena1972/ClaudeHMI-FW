idf_component_register(
    SRCS "st7796.c" "ft5x06.c"
    INCLUDE_DIRS "include"
    REQUIRES 
        driver 
        freertos 
        esp_driver_ledc
        esp_lcd
        esp_lcd_st7796
        esp_lcd_touch
        esp_lcd_touch_ft5x06 
        lvgl 
        esp_lvgl_port
    #esp_driver_i2c
    PRIV_REQUIRES 
        esp_timer 
         
        nvs_flash
)
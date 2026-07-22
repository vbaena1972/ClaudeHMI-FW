#Add sources from ui directory
file(GLOB_RECURSE SRC_UI ${CMAKE_SOURCE_DIR} "ui/*.c")

idf_component_register(SRCS "main.c" ${SRC_UI}
                    INCLUDE_DIRS "." "ui"
                    REQUIRES 
                         bt
                         cert_store
                         cjson
                         diag
                         display 
                         drivers
                         esp_driver_ledc
                         esp_event 
                         esp_http_server
                         esp_sysview 
                         esp_trace
                         esp_lcd
                         esp_netif
                         lvgl 
                         network_core
                         nvs_flash 
                         sensors_runtime
                         state_pub
                         storage 
                         transport_ble
                         transport_http
                         ui_bind
                         task_tracer
                         )

idf_component_register(
    SRCS "sensors_runtime.c"
    INCLUDE_DIRS "include"
    REQUIRES 
        freertos 
        esp_timer
    PRIV_REQUIRES
        storage 
)
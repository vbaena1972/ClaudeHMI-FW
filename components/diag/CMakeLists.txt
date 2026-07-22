idf_component_register(
    SRCS "mem_diag.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos esp_timer
)
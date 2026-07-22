idf_component_register(
    SRCS "cert_store.c"
    INCLUDE_DIRS "." "include"
    REQUIRES 
        nvs_flash 
        mbedtls 
        fatfs
)

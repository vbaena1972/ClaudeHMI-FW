idf_component_register(
    SRCS "storage.c" "storage_extras.c" "metrics_store.c"
    INCLUDE_DIRS "." "include"
    REQUIRES nvs_flash cjson
    PRIV_REQUIRES esp_timer
    )

idf_component_register(
    SRCS "http_api.c" "ws_stream.c"
    INCLUDE_DIRS "include"
    REQUIRES 
        esp_http_server
        esp_netif 
        cjson 
    PRIV_REQUIRES 
        storage
        app_update  
        state_pub
        diag
)

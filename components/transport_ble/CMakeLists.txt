idf_component_register(
    SRCS "transport_ble.c"
    INCLUDE_DIRS "include"
    REQUIRES 
        bt cjson
    PRIV_REQUIRES 
        storage
)
# Agregar include de NimBLE HCI manualmente:
target_include_directories(${COMPONENT_LIB} PRIVATE
    "$ENV{IDF_PATH}/components/bt/host/nimble/esp-hci/include"
)

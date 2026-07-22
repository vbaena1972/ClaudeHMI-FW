idf_component_register(
    SRCS
        "ui_wifi_main_icon.c" "ui_statusbar_controller.c"
    INCLUDE_DIRS
        "."
        "include"
        "${CMAKE_SOURCE_DIR}/main"
        "${CMAKE_SOURCE_DIR}/main/ui"
    REQUIRES
        lvgl
        network_core
        display
        transport_ble
        storage
        sensors_runtime
        drivers
    )
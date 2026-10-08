idf_component_register(
    SRCS "main.cpp" "display.cpp" "ble.cpp"
    REQUIRES esp_timer bt nvs_flash
    INCLUDE_DIRS "."
)


idf_component_register(
    SRCS "wifi_drv.c"
    INCLUDE_DIRS "."
    REQUIRES
        esp_wifi
        nvs_flash
        driver
)
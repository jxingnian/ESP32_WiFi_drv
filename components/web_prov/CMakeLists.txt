 idf_component_register(
    SRCS "web_prov.c"
    INCLUDE_DIRS "."
    REQUIRES spiffs_mgr wifi_drv esp_http_server
)
spiffs_create_partition_image(spiffs ./spiffs_image FLASH_IN_PROJECT)
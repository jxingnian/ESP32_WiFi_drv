#include "spiffs_mgr.h"
#include "esp_log.h"
#include "esp_spiffs.h"

static const char *TAG = "spiffs_mgr";
static bool s_spiffs_mounted = false;

esp_err_t spiffs_mgr_mount(void) {
    if (s_spiffs_mounted) return ESP_OK;
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 8,
        .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret == ESP_OK) {
        s_spiffs_mounted = true;
        ESP_LOGI(TAG, "SPIFFS mounted");
    } else {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t spiffs_mgr_unmount(void) {
    if (!s_spiffs_mounted) return ESP_OK;
    esp_err_t ret = esp_vfs_spiffs_unregister(NULL);
    if (ret == ESP_OK) {
        s_spiffs_mounted = false;
        ESP_LOGI(TAG, "SPIFFS unmounted");
    } else {
        ESP_LOGE(TAG, "SPIFFS unmount failed: %s", esp_err_to_name(ret));
    }
    return ret;
}
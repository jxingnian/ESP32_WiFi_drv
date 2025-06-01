/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-05-31 19:26:35
 * @LastEditors: 星年 && j_xingnian@163.com
 * @LastEditTime: 2025-06-01 16:11:27
 * @FilePath: \hello_world\main\hello_world_main.c
 * @Description:
 *
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved.
 */
/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-05-31 19:26:35
 * @LastEditors: 星年 && j_xingnian@163.com
 * @LastEditTime: 2025-06-01 14:56:20
 * @FilePath: \hello_world\main\hello_world_main.c
 * @Description: 全方位测试WiFi驱动功能
 *
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved.
 */
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "wifi_drv.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "web_prov.h"

static const char *TAG = "MAIN-----MAIN-----";

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // 1. 初始化WiFi
    ESP_LOGI(TAG, "初始化WiFi...");
    wifi_drv_init();

    // ESP_LOGI(TAG, "连接AP...");
    // wifi_drv_connect("HOME", "15975324685");
    // // vTaskDelay(2000 / portTICK_PERIOD_MS); // 延时2秒

    web_prov_start();

    // // 3. 断开连接
    // ESP_LOGI(TAG, "断开AP连接...");
    // wifi_drv_disconnect();
    // vTaskDelay(1000 / portTICK_PERIOD_MS); // 延时1秒

    // // 4. 切换到APSTA模式并连接
    // ESP_LOGI(TAG, "切换到APSTA模式并连接...");
    // wifi_drv_set_mode(WIFI_MODE_APSTA);
    // ESP_LOGI(TAG, "连接AP...");
    // wifi_drv_connect("HOME", "15975324685");
    // vTaskDelay(2000 / portTICK_PERIOD_MS); // 延时2秒

    // // 5. 断开连接
    // ESP_LOGI(TAG, "断开APSTA连接...");
    // wifi_drv_disconnect();
    // vTaskDelay(1000 / portTICK_PERIOD_MS); // 延时1秒

    // // 6. WiFi扫描测试
    // wifi_ap_record_t *ap_list = NULL;
    // uint16_t ap_num = 0;
    // ESP_LOGI(TAG, "开始扫描周围WiFi...");
    // ret = wifi_drv_scan(&ap_list, &ap_num);
    // if (ret == ESP_OK && ap_list != NULL) {
    //     ESP_LOGI(TAG, "扫描到 %d 个AP：", ap_num);
    //     for (int i = 0; i < ap_num; ++i) {
    //         ESP_LOGI(TAG, "[%d] SSID: %s, RSSI: %d", i + 1, (char *)ap_list[i].ssid, ap_list[i].rssi);
    //     }
    //     free(ap_list);
    // } else {
    //     ESP_LOGE(TAG, "WiFi扫描失败，错误码: %d", ret);
    // }

    // // 7. 测试保存WiFi信息到NVS
    // wifi_ap_info_t ap_info = {0};
    // snprintf(ap_info.ssid, sizeof(ap_info.ssid), "HOME");
    // snprintf(ap_info.password, sizeof(ap_info.password), "15975324685");
    // ESP_LOGI(TAG, "保存WiFi信息到NVS...");
    // ret = wifi_drv_save_ap(&ap_info);
    // if (ret == ESP_OK) {
    //     ESP_LOGI(TAG, "WiFi信息保存成功");
    // } else {
    //     ESP_LOGE(TAG, "WiFi信息保存失败，错误码: %d", ret);
    // }

    // // 8. 测试从NVS读取WiFi信息
    // wifi_ap_info_t ap_info_read = {0};
    // ESP_LOGI(TAG, "从NVS读取WiFi信息...");
    // ret = wifi_drv_get_all_ap(&ap_info_read);
    // if (ret == ESP_OK) {
    //     ESP_LOGI(TAG, "读取到WiFi信息: SSID=%s, PASSWORD=%s", ap_info_read.ssid, ap_info_read.password);
    // } else {
    //     ESP_LOGE(TAG, "读取WiFi信息失败，错误码: %d", ret);
    // }

    // // 9. 测试删除NVS中的WiFi信息
    // ESP_LOGI(TAG, "删除NVS中的WiFi信息...");
    // ret = wifi_drv_erase_ap();
    // if (ret == ESP_OK) {
    //     ESP_LOGI(TAG, "WiFi信息删除成功");
    // } else {
    //     ESP_LOGE(TAG, "WiFi信息删除失败，错误码: %d", ret);
    // }

    // ESP_LOGI(TAG, "全方位WiFi驱动测试完成");
}

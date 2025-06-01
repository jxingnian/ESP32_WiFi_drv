/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-05-31 19:55:32
 * @LastEditors: 星年 && j_xingnian@163.com
 * @LastEditTime: 2025-06-01 16:12:48
 * @FilePath: \hello_world\components\wifi_drv\wifi_drv.h
 * @Description:
 *
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved.
 */
#ifndef WIFI_DRV_H
#define WIFI_DRV_H

#include "esp_wifi.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif
#define WIFI_AP_MAX_NUM 5
typedef struct {
    char ssid[32];
    char password[64];
} wifi_ap_info_t;

// 初始化WiFi（STA+AP模式均可用）
esp_err_t wifi_drv_init(void);

// 连接指定WiFi（STA模式）
esp_err_t wifi_drv_connect(const char *ssid, const char *password, wifi_mode_t mode);

// 断开当前WiFi连接
esp_err_t wifi_drv_disconnect(void);

// 扫描WiFi，返回AP列表和数量（需手动free）
esp_err_t wifi_drv_scan(wifi_ap_record_t **ap_list, uint16_t *ap_num);

// 切换WiFi模式{WIFI_MODE_APSTA WIFI_MODE_STA WIFI_MODE_AP WIFI_MODE_NULL}
esp_err_t wifi_drv_set_mode(wifi_mode_t mode);

// 配置AP热点信息并启动APSTA模式
esp_err_t wifi_drv_config_apsta(const char *ssid, const char *password, uint8_t channel, uint8_t max_connection);

// 保存WiFi信息到NVS
esp_err_t wifi_drv_save_ap(const wifi_ap_info_t *ap_info);

// 获取所有已保存WiFi，返回数量
int wifi_drv_get_all_ap(wifi_ap_info_t *list, int max_num);


// 删除NVS中保存的WiFi信息
esp_err_t wifi_drv_erase_ap(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_DRV_H
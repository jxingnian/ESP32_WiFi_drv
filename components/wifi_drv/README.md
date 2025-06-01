<!--
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-05-31 19:55:32
 * @LastEditors: 星年 && j_xingnian@163.com
 * @LastEditTime: 2025-06-01 16:49:34
 * @FilePath: \hello_world\components\wifi_drv\README.md
 * @Description: wifi_drv
 * 
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved. 
-->
# wifi_drv 组件说明

## 功能简介

`wifi_drv` 组件封装了 ESP32 WiFi 的常用操作，包括初始化、连接、断开、扫描、NVS 多组 WiFi 信息的保存/获取/删除等，便于上层应用调用。

## 主要接口

- `esp_err_t wifi_drv_init(void);`  
  初始化 WiFi 驱动（需先初始化 NVS）

- `esp_err_t wifi_drv_connect(const char *ssid, const char *password, wifi_mode_t mode);`  
  连接指定 WiFi（支持 STA/APSTA 模式）

- `esp_err_t wifi_drv_disconnect(void);`  
  断开当前 WiFi 连接

- `esp_err_t wifi_drv_scan(wifi_ap_record_t **ap_list, uint16_t *ap_num);`  
  扫描附近 WiFi，返回 AP 列表（需手动 free）

- `esp_err_t wifi_drv_set_mode(wifi_mode_t mode);`  
  设置 WiFi 工作模式

- `esp_err_t wifi_drv_config_apsta(const char *ssid, const char *password, uint8_t channel, uint8_t max_connection);`  
  配置 APSTA 模式下的 AP 信息

- `esp_err_t wifi_drv_save_ap(const wifi_ap_info_t *ap_info);`  
  保存一组 WiFi 信息到 NVS（最多 5 组，自动去重/覆盖）

- `int wifi_drv_get_all_ap(wifi_ap_info_t *list, int max_num);`  
  获取所有已保存的 WiFi 信息

- `esp_err_t wifi_drv_erase_ap(void);`  
  删除所有已保存的 WiFi 信息

## 典型用法

```c
nvs_flash_init();
wifi_drv_init();

wifi_ap_info_t info = { .ssid = "xxx", .password = "yyy" };
wifi_drv_save_ap(&info);

wifi_ap_info_t list[5];
int num = wifi_drv_get_all_ap(list, 5);

wifi_drv_connect(list[0].ssid, list[0].password, WIFI_MODE_STA);

wifi_drv_disconnect();
wifi_drv_erase_ap();
```

## 注意事项

- 使用前需先初始化 NVS（`nvs_flash_init()`）。
- 扫描结果需手动释放内存。
- NVS 最多保存 5 组 WiFi 信息，超出会自动覆盖最早的。
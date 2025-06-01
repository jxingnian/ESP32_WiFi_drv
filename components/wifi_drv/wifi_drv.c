/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-05-31 19:55:32
 * @LastEditors: 星年 && j_xingnian@163.com
 * @LastEditTime: 2025-06-01 09:52:51
 * @FilePath: \hello_world\components\wifi_drv\wifi_drv.c
 * @Description: WiFi 驱动实现，包含初始化、连接、扫描、NVS存储等功能
 *
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved.
 */
#include "wifi_drv.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

#define WIFI_NVS_NAMESPACE "wifi_cfg"   // NVS命名空间
#define WIFI_NVS_KEY "ap_info"          // NVS中保存AP信息的键

static const char *TAG = "wifi_drv";    // 日志标签

static bool s_wifi_inited = false;      // WiFi初始化标志

/**
 * @brief 初始化WiFi驱动
 *
 * 该函数完成WiFi驱动的初始化，包括网络接口、事件循环、WiFi驱动本身的初始化等。
 * 只会初始化一次，后续调用直接返回。
 *
 * @return esp_err_t
 */
esp_err_t wifi_drv_init(void)
{
    if (s_wifi_inited) return ESP_OK; // 已初始化则直接返回
    ESP_ERROR_CHECK(esp_netif_init()); // 初始化网络接口
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // 创建默认事件循环
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // 获取默认WiFi配置
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)); // 初始化WiFi驱动
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM)); // 设置WiFi配置存储在RAM
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL)); // 设置WiFi模式为NULL
    ESP_ERROR_CHECK(esp_wifi_start()); // 启动WiFi
    s_wifi_inited = true; // 标记已初始化
    return ESP_OK;
}

/**
 * @brief 反初始化WiFi驱动
 *
 * 停止并释放WiFi驱动资源。
 *
 * @return esp_err_t
 */
esp_err_t wifi_drv_deinit(void)
{
    if (!s_wifi_inited) return ESP_OK; // 未初始化则直接返回
    ESP_ERROR_CHECK(esp_wifi_stop()); // 停止WiFi
    ESP_ERROR_CHECK(esp_wifi_deinit()); // 反初始化WiFi驱动
    s_wifi_inited = false; // 标记未初始化
    return ESP_OK;
}

/**
 * @brief 连接到指定WiFi
 *
 * 设置STA模式并连接到指定的SSID和密码的AP。
 *
 * @param ssid      目标AP的SSID
 * @param password  目标AP的密码（可为NULL）
 * @return esp_err_t
 */
esp_err_t wifi_drv_connect(const char *ssid, const char *password)
{
    wifi_config_t wifi_config = {0}; // 初始化WiFi配置结构体
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid)); // 拷贝SSID
    if (password) {
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password)); // 拷贝密码
    }
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // 设置为STA模式
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config)); // 设置STA配置
    return esp_wifi_connect(); // 发起连接
}

/**
 * @brief 断开当前WiFi连接
 *
 * @return esp_err_t
 */
esp_err_t wifi_drv_disconnect(void)
{
    return esp_wifi_disconnect(); // 断开WiFi连接
}

/**
 * @brief 扫描周围WiFi AP
 *
 * 扫描周围可用的WiFi AP，并返回AP列表和数量。调用者需负责释放ap_list内存。
 *
 * @param ap_list   输出参数，指向AP记录数组的指针
 * @param ap_num    输出参数，AP数量
 * @return esp_err_t
 */
esp_err_t wifi_drv_scan(wifi_ap_record_t **ap_list, uint16_t *ap_num)
{
    wifi_scan_config_t scan_config = {0}; // 初始化扫描配置
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // 设置为STA模式
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true)); // 启动同步扫描
    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count)); // 获取AP数量
    if (ap_count == 0) {
        *ap_list = NULL;
        *ap_num = 0;
        return ESP_OK; // 未扫描到AP
    }
    wifi_ap_record_t *list = malloc(sizeof(wifi_ap_record_t) * ap_count); // 分配AP记录数组
    if (!list) return ESP_ERR_NO_MEM; // 内存分配失败
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, list)); // 获取AP记录
    *ap_list = list; // 返回AP列表
    *ap_num = ap_count; // 返回AP数量
    return ESP_OK;
}

/**
 * @brief 设置WiFi工作模式
 *
 * @param mode  WiFi模式（STA/AP/STA+AP/NULL）
 * @return esp_err_t
 */
esp_err_t wifi_drv_set_mode(wifi_mode_t mode)
{
    return esp_wifi_set_mode(mode); // 设置WiFi模式
}

/**
 * @brief 保存AP信息到NVS
 *
 * 将AP信息结构体保存到NVS中，便于下次启动时恢复连接。
 *
 * @param ap_info   需要保存的AP信息
 * @return esp_err_t
 */
esp_err_t wifi_drv_save_ap(const wifi_ap_info_t *ap_info)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle); // 打开NVS
    if (err != ESP_OK) return err;
    err = nvs_set_blob(nvs_handle, WIFI_NVS_KEY, ap_info, sizeof(wifi_ap_info_t)); // 写入AP信息
    if (err == ESP_OK) err = nvs_commit(nvs_handle); // 提交更改
    nvs_close(nvs_handle); // 关闭NVS
    return err;
}

/**
 * @brief 从NVS读取AP信息
 *
 * 读取之前保存的AP信息，用于自动重连等场景。
 *
 * @param ap_info   输出参数，读取到的AP信息
 * @return esp_err_t
 */
esp_err_t wifi_drv_get_ap(wifi_ap_info_t *ap_info)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READONLY, &nvs_handle); // 只读方式打开NVS
    if (err != ESP_OK) return err;
    size_t len = sizeof(wifi_ap_info_t);
    err = nvs_get_blob(nvs_handle, WIFI_NVS_KEY, ap_info, &len); // 读取AP信息
    nvs_close(nvs_handle); // 关闭NVS
    return err;
}

/**
 * @brief 擦除NVS中的AP信息
 *
 * 删除NVS中保存的AP信息。
 *
 * @return esp_err_t
 */
esp_err_t wifi_drv_erase_ap(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle); // 读写方式打开NVS
    if (err != ESP_OK) return err;
    err = nvs_erase_key(nvs_handle, WIFI_NVS_KEY); // 擦除AP信息
    if (err == ESP_OK) err = nvs_commit(nvs_handle); // 提交更改
    nvs_close(nvs_handle); // 关闭NVS
    return err;
}
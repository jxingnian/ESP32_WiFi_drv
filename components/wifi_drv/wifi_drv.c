/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-05-31 19:55:32
 * @LastEditors: 星年 && j_xingnian@163.com
 * @LastEditTime: 2025-06-01 13:06:44
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
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

// NVS命名空间，用于存储WiFi配置
#define WIFI_NVS_NAMESPACE "wifi_cfg"
// NVS中保存AP信息的键名
#define WIFI_NVS_KEY "ap_info"

// 日志标签
static const char *TAG = "wifi_drv";

// WiFi初始化标志，防止重复初始化
static bool s_wifi_inited = false;
// WiFi已启动标志
static bool s_wifi_started = false;
// 全局互斥锁，保护WiFi相关操作的并发安全
static SemaphoreHandle_t s_wifi_mutex = NULL;
// WiFi事件组，用于同步WiFi事件（如连接、断开、扫描完成等）
static EventGroupHandle_t s_wifi_event_group = NULL;

// 事件组中的各个事件位定义
#define WIFI_CONNECTED_BIT BIT0      // 已连接
#define WIFI_FAIL_BIT      BIT1      // 连接失败
#define WIFI_STARTED_BIT   BIT2      // WiFi已启动
#define WIFI_SCAN_DONE_BIT BIT3      // 扫描完成

// 事件处理器实例句柄
static esp_event_handler_instance_t instance_any_id = NULL;
static esp_event_handler_instance_t instance_got_ip = NULL;

/**
 * @brief WiFi/网络事件处理函数
 *
 * 该函数处理WiFi和IP相关的事件，包括：
 * - STA启动
 * - STA断开
 * - STA获取IP
 * - 扫描完成
 *
 * @param arg        用户参数（未使用）
 * @param event_base 事件基类型（WIFI_EVENT/IP_EVENT）
 * @param event_id   事件ID
 * @param event_data 事件数据
 */
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    // STA模式启动事件
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
        s_wifi_started = true;
        // 设置WiFi已启动事件位
        xEventGroupSetBits(s_wifi_event_group, WIFI_STARTED_BIT);
    }
    // STA断开事件
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    // STA获取到IP事件
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        // 设置已连接事件位
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
    // WiFi扫描完成事件
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "WIFI_EVENT_SCAN_DONE");
        // 设置扫描完成事件位
        xEventGroupSetBits(s_wifi_event_group, WIFI_SCAN_DONE_BIT);
    }
}

/**
 * @brief 初始化WiFi驱动
 *
 * 该函数完成WiFi驱动的初始化，包括：
 * - 创建互斥锁和事件组
 * - 初始化网络接口
 * - 创建默认STA网络接口
 * - 注册WiFi和IP事件处理器
 * - 初始化WiFi驱动
 * 只会初始化一次，后续调用直接返回。
 *
 * @return esp_err_t  ESP_OK表示成功，其他为失败
 */
esp_err_t wifi_drv_init(void)
{
    esp_err_t err = ESP_OK;

    // 创建互斥锁（仅创建一次）
    if (s_wifi_mutex == NULL) {
        s_wifi_mutex = xSemaphoreCreateMutex();
        if (s_wifi_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create WiFi mutex");
            return ESP_FAIL;
        }
    }
    // 创建事件组（仅创建一次）
    if (s_wifi_event_group == NULL) {
        s_wifi_event_group = xEventGroupCreate();
        if (s_wifi_event_group == NULL) {
            ESP_LOGE(TAG, "Failed to create WiFi event group");
            return ESP_FAIL;
        }
    }

    // 进入临界区，防止并发初始化
    xSemaphoreTake(s_wifi_mutex, portMAX_DELAY);
    if (s_wifi_inited) {
        // 已初始化，直接返回
        xSemaphoreGive(s_wifi_mutex);
        return ESP_OK;
    }

    // 初始化网络接口
    ESP_ERROR_CHECK(esp_netif_init());
    // 创建默认事件循环
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // 创建默认STA网络接口
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // 注册WiFi事件处理器（所有WiFi事件）
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID,
                    &event_handler,
                    NULL,
                    &instance_any_id));
    // 注册IP事件处理器（仅获取IP事件）
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                    IP_EVENT_STA_GOT_IP,
                    &event_handler,
                    NULL,
                    &instance_got_ip));

    // 初始化WiFi驱动
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 标记已初始化
    s_wifi_inited = true;
    s_wifi_started = false;
    xSemaphoreGive(s_wifi_mutex);
    ESP_LOGI(TAG, "WiFi init");
    return err;
}

/**
 * @brief 启动WiFi
 *
 * 若WiFi未初始化则先初始化，若已启动则直接返回。
 * 启动后等待WIFI_STARTED_BIT事件，超时则返回错误。
 *
 * @return esp_err_t  ESP_OK表示成功，其他为失败
 */
static esp_err_t wifi_drv_start(void)
{
    esp_err_t err = ESP_OK;

    // 若未初始化，先初始化
    if (!s_wifi_inited) {
        err = wifi_drv_init();
        if (err != ESP_OK) return err;
    }
    // 若已启动，直接返回
    if (s_wifi_started) return ESP_OK;

    // 启动WiFi
    err = esp_wifi_start();
    if (err == ESP_OK) {
        // 等待WiFi启动事件（最长5秒）
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_STARTED_BIT, pdTRUE, pdFALSE, pdMS_TO_TICKS(5000));
        if (!(bits & WIFI_STARTED_BIT)) {
            ESP_LOGE(TAG, "WiFi start timeout");
            return ESP_ERR_TIMEOUT;
        }
        s_wifi_started = true;
    }
    return err;
}

/**
 * @brief 关闭WiFi
 *
 * 若WiFi未启动则直接返回。关闭后清除相关事件位。
 *
 * @return esp_err_t  ESP_OK表示成功，其他为失败
 */
esp_err_t wifi_drv_stop(void)
{
    esp_err_t err = ESP_OK;
    if (!s_wifi_started) {
        // 未启动，直接返回
        xSemaphoreGive(s_wifi_mutex);
        return ESP_OK;
    }
    // 停止WiFi
    err = esp_wifi_stop();
    if (err == ESP_OK) {
        s_wifi_started = false;
        // 清除启动、连接、失败事件位
        xEventGroupClearBits(s_wifi_event_group, WIFI_STARTED_BIT | WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    }
    return err;
}

/**
 * @brief 连接到指定WiFi
 *
 * 设置STA模式并连接到指定的SSID和密码的AP。
 * 阻塞等待连接结果（成功/失败/超时）。
 *
 * @param ssid      目标AP的SSID
 * @param password  目标AP的密码（可为NULL）
 * @return esp_err_t
 */
esp_err_t wifi_drv_connect(const char *ssid, const char *password)
{
    esp_err_t err = ESP_OK;
    // 检查SSID有效性
    if (ssid == NULL || strlen(ssid) == 0) return ESP_ERR_INVALID_ARG;

    // 确保WiFi初始化
    if (!s_wifi_inited) {
        err = wifi_drv_init();
        if (err != ESP_OK) {
            return err;
        }
    }

    // 进入临界区
    xSemaphoreTake(s_wifi_mutex, portMAX_DELAY);

    // 设置为STA模式
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // 配置WiFi参数
    wifi_config_t wifi_config = { 0 };
    strlcpy((char *) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    if (password) {
        strlcpy((char *) wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    }
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // 启动WiFi（如未启动）
    err = wifi_drv_start();
    if (err != ESP_OK) {
        xSemaphoreGive(s_wifi_mutex);
        return err;
    }

    // 清除连接相关事件位
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    // 重置重试次数
    s_retry_num = 0;
    // 发起连接
    ESP_ERROR_CHECK(esp_wifi_connect());

    // 阻塞等待连接结果（10秒超时）
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdTRUE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(10000));
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP: %s", ssid);
        err = ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to AP: %s", ssid);
        err = ESP_FAIL;
    } else {
        ESP_LOGE(TAG, "WiFi connect timeout");
        err = ESP_ERR_TIMEOUT;
    }
    xSemaphoreGive(s_wifi_mutex);
    return err;
}

/**
 * @brief 断开当前WiFi连接，并关闭WiFi
 *
 * 若WiFi未启动则直接返回。断开后延迟一段时间再关闭WiFi。
 *
 * @return esp_err_t
 */
esp_err_t wifi_drv_disconnect(void)
{
    esp_err_t err = ESP_OK;
    // 进入临界区
    xSemaphoreTake(s_wifi_mutex, portMAX_DELAY);
    if (!s_wifi_started) {
        // 未启动，直接返回
        xSemaphoreGive(s_wifi_mutex);
        return ESP_OK;
    }
    // 断开WiFi连接
    err = esp_wifi_disconnect();
    if (err == ESP_OK) {
        // 阻塞等待断开事件（2秒超时）
        EventBits_t bits = xEventGroupWaitBits(
                               s_wifi_event_group,
                               WIFI_FAIL_BIT,
                               pdTRUE,   // 清除
                               pdFALSE,  // 任一
                               pdMS_TO_TICKS(2000)
                           );
        if (!(bits & WIFI_FAIL_BIT)) {
            ESP_LOGW(TAG, "WiFi disconnect timeout");
        }
    }
    // 关闭WiFi
    ESP_ERROR_CHECK(wifi_drv_stop());
    xSemaphoreGive(s_wifi_mutex);
    return err;
}

/**
 * @brief 扫描周围WiFi AP
 *
 * 扫描周围可用的WiFi AP，并返回AP列表和数量。
 * 调用者需负责释放ap_list内存。
 *
 * @param ap_list   输出参数，指向AP记录数组的指针
 * @param ap_num    输出参数，AP数量
 * @return esp_err_t
 */
esp_err_t wifi_drv_scan(wifi_ap_record_t **ap_list, uint16_t *ap_num)
{
    esp_err_t err = ESP_OK;
    // 检查参数有效性
    if (ap_list == NULL || ap_num == NULL) return ESP_ERR_INVALID_ARG;

    // 确保WiFi初始化
    if (!s_wifi_inited) {
        err = wifi_drv_init();
        if (err != ESP_OK) {
            return err;
        }
    }
    // 设置为STA模式
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    // 进入临界区
    xSemaphoreTake(s_wifi_mutex, portMAX_DELAY);
    // 启动WiFi
    err = wifi_drv_start();
    if (err != ESP_OK) {
        xSemaphoreGive(s_wifi_mutex);
        return err;
    }
    // 配置扫描参数（默认全通道、被动扫描）
    wifi_scan_config_t scan_config = {0};
    // 清除扫描完成事件位
    xEventGroupClearBits(s_wifi_event_group, WIFI_SCAN_DONE_BIT);
    // 启动异步扫描
    err = esp_wifi_scan_start(&scan_config, false);
    if (err != ESP_OK) {
        xSemaphoreGive(s_wifi_mutex);
        return err;
    }
    // 阻塞等待扫描完成（10秒超时）
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_SCAN_DONE_BIT, pdTRUE, pdFALSE, pdMS_TO_TICKS(10000));
    if (!(bits & WIFI_SCAN_DONE_BIT)) {
        ESP_LOGE(TAG, "WiFi scan timeout");
        xSemaphoreGive(s_wifi_mutex);
        return ESP_ERR_TIMEOUT;
    }
    // 获取AP数量
    uint16_t ap_count = 0;
    err = esp_wifi_scan_get_ap_num(&ap_count);
    if (err != ESP_OK || ap_count == 0) {
        *ap_list = NULL;
        *ap_num = 0;
        xSemaphoreGive(s_wifi_mutex);
        return ESP_OK;
    }
    // 分配AP记录数组
    wifi_ap_record_t *list = malloc(sizeof(wifi_ap_record_t) * ap_count);
    if (!list) {
        xSemaphoreGive(s_wifi_mutex);
        return ESP_ERR_NO_MEM;
    }
    // 获取AP记录
    err = esp_wifi_scan_get_ap_records(&ap_count, list);
    if (err != ESP_OK) {
        free(list);
        xSemaphoreGive(s_wifi_mutex);
        return err;
    }
    *ap_list = list;
    *ap_num = ap_count;
    xSemaphoreGive(s_wifi_mutex);
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
    esp_err_t err = ESP_OK;
    // 确保WiFi初始化
    if (!s_wifi_inited) {
        err = wifi_drv_init();
        if (err != ESP_OK) {
            return err;
        }
    }
    // 进入临界区
    xSemaphoreTake(s_wifi_mutex, portMAX_DELAY);
    // 设置WiFi模式
    err = esp_wifi_set_mode(mode);
    xSemaphoreGive(s_wifi_mutex);
    return err;
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
    // 打开NVS命名空间，读写模式
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return err;
    // 写入AP信息（以blob形式）
    err = nvs_set_blob(nvs_handle, WIFI_NVS_KEY, ap_info, sizeof(wifi_ap_info_t));
    // 提交更改
    if (err == ESP_OK) err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
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
    // 打开NVS命名空间，只读模式
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) return err;
    size_t len = sizeof(wifi_ap_info_t);
    // 读取AP信息（blob）
    err = nvs_get_blob(nvs_handle, WIFI_NVS_KEY, ap_info, &len);
    nvs_close(nvs_handle);
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
    // 打开NVS命名空间，读写模式
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return err;
    // 擦除AP信息键
    err = nvs_erase_key(nvs_handle, WIFI_NVS_KEY);
    // 提交更改
    if (err == ESP_OK) err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    return err;
}
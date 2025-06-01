#include "web_prov.h"
#include "spiffs_mgr.h"
#include "wifi_drv.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "web_prov";
static httpd_handle_t s_httpd = NULL;

// index.html 处理器
static esp_err_t index_get_handler(httpd_req_t *req)
{
    FILE *f = fopen("/spiffs/index.html", "r");
    if (!f) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    char buf[512];
    httpd_resp_set_type(req, "text/html");
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        httpd_resp_send_chunk(req, buf, n);
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// /api/status: 获取连接状态、已保存WiFi（多条）
static esp_err_t status_get_handler(httpd_req_t *req)
{
    char resp[512];
    bool connected = false;
    char ip[32] = "0.0.0.0";
    char connected_ssid[33] = "";
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    if (mode == WIFI_MODE_STA || mode == WIFI_MODE_APSTA) {
        wifi_ap_record_t ap;
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
            connected = true;
            strncpy(connected_ssid, (char*)ap.ssid, sizeof(connected_ssid));
            // 获取STA接口IP
            esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (sta_netif) {
                esp_netif_ip_info_t ip_info;
                if (esp_netif_get_ip_info(sta_netif, &ip_info) == ESP_OK) {
                    snprintf(ip, sizeof(ip), IPSTR, IP2STR(&ip_info.ip));
                }
            }
        }
    }
    // 获取所有已保存WiFi
    wifi_ap_info_t ap_list[WIFI_AP_MAX_NUM] = {0};
    int saved_num = wifi_drv_get_all_ap(ap_list, WIFI_AP_MAX_NUM);
    char saved_list[256] = {0};
    strcat(saved_list, "[");
    for (int i = 0; i < saved_num; ++i) {
        char item[40];
        snprintf(item, sizeof(item), "%s\"%s\"", (i ? "," : ""), ap_list[i].ssid);
        strcat(saved_list, item);
    }
    strcat(saved_list, "]");

    snprintf(resp, sizeof(resp),
             "{\"connected\":%s,\"connected_ssid\":\"%s\",\"ip\":\"%s\",\"saved_list\":%s}",
             connected ? "true" : "false",
             connected_ssid,
             ip,
             saved_list
            );
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}


// /api/scan: 扫描WiFi
static esp_err_t scan_get_handler(httpd_req_t *req)
{
    wifi_ap_record_t *ap_list = NULL;
    uint16_t ap_num = 0;
    wifi_drv_scan(&ap_list, &ap_num);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "{\"ap_list\":[");
    for (int i = 0; i < ap_num; ++i) {
        char item[128];
        snprintf(item, sizeof(item),
                 "%s{\"ssid\":\"%s\",\"rssi\":%d,\"authmode\":%d}",
                 (i ? "," : ""),
                 ap_list[i].ssid, ap_list[i].rssi, ap_list[i].authmode
                );
        httpd_resp_sendstr_chunk(req, item);
    }
    httpd_resp_sendstr_chunk(req, "]}");
    httpd_resp_sendstr_chunk(req, NULL);
    if (ap_list) free(ap_list);
    return ESP_OK;
}

// /api/connect: 连接WiFi
static esp_err_t connect_post_handler(httpd_req_t *req)
{
    char buf[128] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf)-1);
    if (len <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    // 解析 JSON
    char ssid[32] = {0}, password[64] = {0};
    char *ssid_p = strstr(buf, "\"ssid\"");
    char *pwd_p = strstr(buf, "\"password\"");
    if (ssid_p) sscanf(ssid_p, "\"ssid\":\"%31[^\"]\"", ssid);
    if (pwd_p) sscanf(pwd_p, "\"password\":\"%63[^\"]\"", password);

    if (strlen(ssid) == 0) {
        httpd_resp_sendstr(req, "SSID不能为空");
        return ESP_OK;
    }

    // 保存到NVS
    wifi_ap_info_t info = {0};
    strncpy(info.ssid, ssid, sizeof(info.ssid));
    strncpy(info.password, password, sizeof(info.password));

    // 连接新WiFi
    esp_err_t err = wifi_drv_connect(ssid, password, WIFI_MODE_APSTA);
    if (err == ESP_OK) {
        httpd_resp_sendstr(req, "WiFi连接成功！");
        wifi_drv_save_ap(&info);
    } else {
        httpd_resp_sendstr(req, "WiFi连接失败，请检查信息");
    }
    return ESP_OK;
}

// /api/saved: 删除已保存WiFi
static esp_err_t saved_delete_handler(httpd_req_t *req)
{
    wifi_drv_erase_ap();
    httpd_resp_sendstr(req, "已删除保存的WiFi信息");
    return ESP_OK;
}

// /api/disconnect: 断开WiFi
static esp_err_t disconnect_post_handler(httpd_req_t *req)
{
    wifi_drv_disconnect();
    httpd_resp_sendstr(req, "WiFi已断开");
    return ESP_OK;
}

// /api/connect_saved: 连接已保存的WiFi（只传ssid，后端查找密码）
static esp_err_t connect_saved_post_handler(httpd_req_t *req)
{
    char buf[128] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf)-1);
    if (len <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    // 解析 JSON
    char ssid[32] = {0};
    char *ssid_p = strstr(buf, "\"ssid\"");
    if (ssid_p) sscanf(ssid_p, "\"ssid\":\"%31[^\"]\"", ssid);

    if (strlen(ssid) == 0) {
        httpd_resp_sendstr(req, "SSID不能为空");
        return ESP_OK;
    }

    // 从NVS查找对应WiFi信息
    wifi_ap_info_t ap_list[WIFI_AP_MAX_NUM] = {0};
    int saved_num = wifi_drv_get_all_ap(ap_list, WIFI_AP_MAX_NUM);
    wifi_ap_info_t *found = NULL;
    for (int i = 0; i < saved_num; ++i) {
        if (strcmp(ap_list[i].ssid, ssid) == 0) {
            found = &ap_list[i];
            break;
        }
    }
    if (!found) {
        httpd_resp_sendstr(req, "未找到保存的WiFi信息");
        return ESP_OK;
    }

    // 用保存的密码连接
    esp_err_t err = wifi_drv_connect(found->ssid, found->password, WIFI_MODE_APSTA);
    if (err == ESP_OK) {
        httpd_resp_sendstr(req, "WiFi连接成功！");
    } else {
        httpd_resp_sendstr(req, "WiFi连接失败，请检查信息");
    }
    return ESP_OK;
}

// 启动HTTP服务器
static void start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    if (httpd_start(&s_httpd, &config) == ESP_OK) {
        // 网页
        httpd_uri_t index_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = index_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(s_httpd, &index_uri);

        // API
        httpd_uri_t status_uri = {
            .uri = "/api/status",
            .method = HTTP_GET,
            .handler = status_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(s_httpd, &status_uri);

        httpd_uri_t scan_uri = {
            .uri = "/api/scan",
            .method = HTTP_GET,
            .handler = scan_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(s_httpd, &scan_uri);

        httpd_uri_t connect_uri = {
            .uri = "/api/connect",
            .method = HTTP_POST,
            .handler = connect_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(s_httpd, &connect_uri);

        httpd_uri_t saved_uri = {
            .uri = "/api/saved",
            .method = HTTP_DELETE,
            .handler = saved_delete_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(s_httpd, &saved_uri);

        httpd_uri_t disconnect_uri = {
            .uri = "/api/disconnect",
            .method = HTTP_POST,
            .handler = disconnect_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(s_httpd, &disconnect_uri);

        httpd_uri_t connect_saved_uri = {
            .uri = "/api/connect_saved",
            .method = HTTP_POST,
            .handler = connect_saved_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(s_httpd, &connect_saved_uri);
    }
}

// 停止HTTP服务器
static void stop_http_server(void)
{
    if (s_httpd) {
        httpd_stop(s_httpd);
        s_httpd = NULL;
    }
}

// 启动网页配网
void web_prov_start(void)
{
    // 1. 挂载SPIFFS
    if (spiffs_mgr_mount() != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS挂载失败");
        return;
    }
    // 2. 启动AP模式
    wifi_drv_config_apsta("ChunFeng", "12345678", 6, 4);

    // 3. 启动HTTP服务器
    start_http_server();
    ESP_LOGI(TAG, "网页配网已启动");
}

// 关闭网页配网
void web_prov_stop(void)
{
    stop_http_server();
    wifi_drv_disconnect();
    spiffs_mgr_unmount();
    ESP_LOGI(TAG, "网页配网已关闭");
}
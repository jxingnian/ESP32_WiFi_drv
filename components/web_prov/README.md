<!--
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-06-01 16:48:20
 * @LastEditors: 星年 && j_xingnian@163.com
 * @LastEditTime: 2025-06-01 16:49:16
 * @FilePath: \hello_world\components\web_prov\README.md
 * @Description: web_prov
 * 
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved. 
-->
# web_prov 组件说明

## 功能简介

`web_prov` 组件用于 ESP32 的网页配网，支持通过 Web 页面扫描、连接、保存、删除 WiFi，并可管理多组 WiFi 配置信息。用户可通过手机/电脑连接 ESP32 的热点，访问内置网页进行 WiFi 配置。

## 主要接口

- `void web_prov_start(void);`  
  启动网页配网（挂载 SPIFFS，启动 AP，HTTP 服务器）

- `void web_prov_stop(void);`  
  关闭网页配网（停止 HTTP 服务器，断开 WiFi，卸载 SPIFFS）

## 主要 API 路由

- `/`：主页（index.html）
- `/api/status`：获取当前连接状态和已保存 WiFi 列表
- `/api/scan`：扫描附近 WiFi
- `/api/connect`：连接指定 WiFi（需提供 ssid 和 password）
- `/api/connect_saved`：连接已保存的 WiFi（只需 ssid，密码由后端查找）
- `/api/saved`：删除已保存的 WiFi（支持按 ssid 删除或全部删除）
- `/api/disconnect`：断开当前 WiFi 连接

## 典型用法

1. 在 `app_main` 中调用 `web_prov_start()` 启动网页配网。
2. 用户通过手机/电脑连接 ESP32 热点，访问网页进行 WiFi 配置。
3. 配网完成后可调用 `web_prov_stop()` 关闭网页配网。

## 注意事项

- 依赖 `spiffs_mgr` 组件用于 SPIFFS 挂载。
- 依赖 `wifi_drv` 组件进行 WiFi 操作。
- 前端网页文件需打包进 SPIFFS 分区。
<!--
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-06-01 16:48:22
 * @LastEditors: 星年 && j_xingnian@163.com
 * @LastEditTime: 2025-06-01 16:48:56
 * @FilePath: \hello_world\components\spiffs_mgr\README.md
 * @Description: spiffs_mgr
 * 
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved. 
-->
# spiffs_mgr 组件说明

## 功能简介

`spiffs_mgr` 组件用于简化 ESP32 上 SPIFFS 文件系统的挂载与卸载操作，便于网页配网等场景下的文件管理。

## 主要接口

- `esp_err_t spiffs_mgr_mount(void);`  
  挂载 SPIFFS 文件系统（多次调用只挂载一次）

- `esp_err_t spiffs_mgr_unmount(void);`  
  卸载 SPIFFS 文件系统

## 典型用法

```c
spiffs_mgr_mount();
// 读写 /spiffs 路径下的文件
spiffs_mgr_unmount();
```

## 注意事项

- 依赖 ESP-IDF 的 SPIFFS 组件。
- 挂载失败时可通过日志查看详细错误信息。
- 建议在系统启动时挂载，退出时卸载。
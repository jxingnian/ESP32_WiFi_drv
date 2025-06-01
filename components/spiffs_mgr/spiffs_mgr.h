#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// 挂载SPIFFS（多次调用只挂载一次）
esp_err_t spiffs_mgr_mount(void);

// 卸载SPIFFS
esp_err_t spiffs_mgr_unmount(void);

#ifdef __cplusplus
}
#endif
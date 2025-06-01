/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-05-31 19:26:35
 * @LastEditors: 星年 && j_xingnian@163.com
 * @LastEditTime: 2025-06-01 10:00:04
 * @FilePath: \hello_world\main\hello_world_main.c
 * @Description:
 *
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved.
 */
/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
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



void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    wifi_drv_init();

    // wifi_drv_set_mode(WIFI_MODE_STA);

    wifi_drv_connect("HOME",15975324685);
}

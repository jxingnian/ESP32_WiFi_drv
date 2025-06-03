/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-06-02 09:06:40
 * @LastEditors: 星年 && j_xingnian@163.com
 * @LastEditTime: 2025-06-02 16:39:13
 * @FilePath: \hello_world\components\inmp441\include\inmp441.h
 * @Description:
 *
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved.
 */
// inmp441.h
#pragma once

#include "driver/i2s_std.h"
#include "esp_err.h"

// INMP441配置结构体
typedef struct {
    i2s_port_t port_num;        // I2S端口号
    int sck_io_num;             // SCK IO编号
    int ws_io_num;              // WS IO编号
    int sd_io_num;              // SD IO编号
    uint32_t sample_rate;       // 采样率
    uint8_t bits_per_sample;    // 采样位数
} inmp441_config_t;

// INMP441设备句柄
typedef struct inmp441_dev_t* inmp441_handle_t;

/**
 * @brief 初始化INMP441
 * @param config 配置参数
 * @param[out] handle 返回的设备句柄
 * @return esp_err_t
 */
esp_err_t inmp441_init(const inmp441_config_t *config, inmp441_handle_t *handle);

/**
 * @brief 读取音频数据
 * @param handle 设备句柄
 * @param[out] data 数据缓冲区
 * @param size 要读取的字节数
 * @param[out] bytes_read 实际读取的字节数
 * @param timeout_ms 超时时间(ms)
 * @return esp_err_t
 */
esp_err_t inmp441_read(inmp441_handle_t handle, void *data, size_t size,
                       size_t *bytes_read, uint32_t timeout_ms);

/**
 * @brief 获取I2S通道句柄
 * @param handle 设备句柄
 * @param[out] chan_handle I2S通道句柄
 * @return esp_err_t
 */
esp_err_t inmp441_get_i2s_chan(inmp441_handle_t handle,
                               i2s_chan_handle_t *chan_handle);

/**
 * @brief 销毁INMP441设备
 * @param handle 设备句柄
 * @return esp_err_t
 */
esp_err_t inmp441_deinit(inmp441_handle_t handle);
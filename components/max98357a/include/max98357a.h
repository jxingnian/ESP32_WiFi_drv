/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-06-02 09:07:37
 * @LastEditors: 星年 && j_xingnian@163.com
 * @LastEditTime: 2025-06-02 16:39:37
 * @FilePath: \hello_world\components\max98357a\include\max98357a.h
 * @Description:
 *
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved.
 */
// max98357a.h
#pragma once

#include "driver/i2s_std.h"
#include "esp_err.h"

// MAX98357A配置结构体
typedef struct {
    i2s_port_t port_num;        // I2S端口号
    int bck_io_num;             // BCK IO编号
    int ws_io_num;              // WS IO编号
    int data_io_num;            // DATA IO编号
    int sd_mode_io_num;         // SD_MODE IO编号(可选)
    uint32_t sample_rate;       // 采样率
    uint8_t bits_per_sample;    // 采样位数
} max98357a_config_t;

// MAX98357A设备句柄
typedef struct max98357a_dev_t* max98357a_handle_t;

/**
 * @brief 初始化MAX98357A
 * @param config 配置参数
 * @param[out] handle 返回的设备句柄
 * @return esp_err_t
 */
esp_err_t max98357a_init(const max98357a_config_t *config,
                         max98357a_handle_t *handle);

/**
 * @brief 写入音频数据
 * @param handle 设备句柄
 * @param data 数据缓冲区
 * @param size 要写入的字节数
 * @param[out] bytes_written 实际写入的字节数
 * @param timeout_ms 超时时间(ms)
 * @return esp_err_t
 */
esp_err_t max98357a_write(max98357a_handle_t handle, const void *data,
                          size_t size, size_t *bytes_written,
                          uint32_t timeout_ms);

/**
 * @brief 设置音量
 * @param handle 设备句柄
 * @param volume 音量值(0-100)
 * @return esp_err_t
 */
esp_err_t max98357a_set_volume(max98357a_handle_t handle, uint8_t volume);

/**
 * @brief 设置静音状态
 * @param handle 设备句柄
 * @param mute true表示静音
 * @return esp_err_t
 */
esp_err_t max98357a_set_mute(max98357a_handle_t handle, bool mute);

/**
 * @brief 获取I2S通道句柄
 * @param handle 设备句柄
 * @param[out] chan_handle I2S通道句柄
 * @return esp_err_t
 */
esp_err_t max98357a_get_i2s_chan(max98357a_handle_t handle,
                                 i2s_chan_handle_t *chan_handle);

/**
 * @brief 销毁MAX98357A设备
 * @param handle 设备句柄
 * @return esp_err_t
 */
esp_err_t max98357a_deinit(max98357a_handle_t handle);
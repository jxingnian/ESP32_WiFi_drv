/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-06-02 09:06:46
 * @LastEditors: 星年 && j_xingnian@163.com
 * @LastEditTime: 2025-06-02 16:39:25
 * @FilePath: \hello_world\components\inmp441\inmp441.c
 * @Description:
 *
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved.
 */
// inmp441.c
#include "inmp441.h"
#include "esp_log.h"

static const char *TAG = "inmp441";

struct inmp441_dev_t {
    i2s_chan_handle_t i2s_chan;     // I2S通道句柄
    uint32_t sample_rate;           // 采样率
    uint8_t bits_per_sample;        // 采样位数
    bool is_initialized;            // 初始化标志
};

esp_err_t inmp441_init(const inmp441_config_t *config, inmp441_handle_t *handle)
{
    esp_err_t ret = ESP_OK;

    // 参数检查
    if (!config || !handle) {
        return ESP_ERR_INVALID_ARG;
    }

    // 分配设备结构内存
    inmp441_handle_t dev = calloc(1, sizeof(struct inmp441_dev_t));
    if (!dev) {
        return ESP_ERR_NO_MEM;
    }

    // 保存配置
    dev->sample_rate = config->sample_rate;
    dev->bits_per_sample = config->bits_per_sample;
    dev->is_initialized = false;

    // I2S标准模式配置
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(config->port_num,
                                 I2S_ROLE_MASTER);
    // 创建I2S通道
    ret = i2s_new_channel(&chan_cfg, NULL, &dev->i2s_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S channel: %d", ret);
        goto err_alloc;
    }

    // I2S标准模式配置
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(config->sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(config->bits_per_sample,
            I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = config->sck_io_num,
            .ws = config->ws_io_num,
            .dout = I2S_GPIO_UNUSED,
            .din = config->sd_io_num,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    // 初始化I2S标准模式
    ret = i2s_channel_init_std_mode(dev->i2s_chan, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init I2S channel: %d", ret);
        goto err_chan;
    }

    // 启动I2S通道
    ret = i2s_channel_enable(dev->i2s_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S channel: %d", ret);
        goto err_chan;
    }

    dev->is_initialized = true;
    *handle = dev;
    ESP_LOGI(TAG, "INMP441 initialized successfully");
    return ESP_OK;

err_chan:
    i2s_del_channel(dev->i2s_chan);
err_alloc:
    free(dev);
    return ret;
}

esp_err_t inmp441_read(inmp441_handle_t handle, void *data, size_t size,
                       size_t *bytes_read, uint32_t timeout_ms)
{
    if (!handle || !handle->is_initialized || !data || !bytes_read) {
        return ESP_ERR_INVALID_ARG;
    }

    return i2s_channel_read(handle->i2s_chan, data, size, bytes_read,
                            timeout_ms);
}

esp_err_t inmp441_get_i2s_chan(inmp441_handle_t handle,
                               i2s_chan_handle_t *chan_handle)
{
    if (!handle || !handle->is_initialized || !chan_handle) {
        return ESP_ERR_INVALID_ARG;
    }

    *chan_handle = handle->i2s_chan;
    return ESP_OK;
}

esp_err_t inmp441_deinit(inmp441_handle_t handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    if (handle->is_initialized) {
        // 禁用I2S通道
        i2s_channel_disable(handle->i2s_chan);
        // 删除I2S通道
        i2s_del_channel(handle->i2s_chan);
        handle->is_initialized = false;
    }

    free(handle);
    ESP_LOGI(TAG, "INMP441 deinitialized");
    return ESP_OK;
}
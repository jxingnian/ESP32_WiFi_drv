// max98357a.c
#include "max98357a.h"
#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "max98357a";

struct max98357a_dev_t {
    i2s_chan_handle_t i2s_chan;     // I2S通道句柄
    int sd_mode_io_num;             // SD_MODE引脚
    uint32_t sample_rate;           // 采样率
    uint8_t bits_per_sample;        // 采样位数
    uint8_t volume;                 // 音量(0-100)
    bool is_muted;                  // 静音状态
    bool is_initialized;            // 初始化标志
};

esp_err_t max98357a_init(const max98357a_config_t *config,
                         max98357a_handle_t *handle)
{
    esp_err_t ret = ESP_OK;

    // 参数检查
    if (!config || !handle) {
        return ESP_ERR_INVALID_ARG;
    }

    // 分配设备结构内存
    max98357a_handle_t dev = calloc(1, sizeof(struct max98357a_dev_t));
    if (!dev) {
        return ESP_ERR_NO_MEM;
    }

    // 保存配置
    dev->sd_mode_io_num = config->sd_mode_io_num;
    dev->sample_rate = config->sample_rate;
    dev->bits_per_sample = config->bits_per_sample;
    dev->volume = 100;
    dev->is_muted = false;
    dev->is_initialized = false;

    // 配置SD_MODE引脚(如果有)
    if (dev->sd_mode_io_num >= 0) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << dev->sd_mode_io_num),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        ret = gpio_config(&io_conf);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure SD_MODE GPIO: %d", ret);
            goto err_alloc;
        }
        gpio_set_level(dev->sd_mode_io_num, 1);
    }

    // I2S标准模式配置
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(config->port_num,
                                 I2S_ROLE_MASTER);
    // 创建I2S通道
    ret = i2s_new_channel(&chan_cfg, &dev->i2s_chan, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S channel: %d", ret);
        goto err_gpio;
    }

    // I2S标准模式配置
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(config->sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(config->bits_per_sample,
            I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = config->bck_io_num,
            .ws = config->ws_io_num,
            .dout = config->data_io_num,
            .din = I2S_GPIO_UNUSED,
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
    ESP_LOGI(TAG, "MAX98357A initialized successfully");
    return ESP_OK;

err_chan:
    i2s_del_channel(dev->i2s_chan);
err_gpio:
    if (dev->sd_mode_io_num >= 0) {
        gpio_reset_pin(dev->sd_mode_io_num);
    }
err_alloc:
    free(dev);
    return ret;
}

esp_err_t max98357a_write(max98357a_handle_t handle, const void *data,
                          size_t size, size_t *bytes_written,
                          uint32_t timeout_ms)
{
    if (!handle || !handle->is_initialized || !data || !bytes_written) {
        return ESP_ERR_INVALID_ARG;
    }

    if (handle->is_muted) {
        *bytes_written = size;
        return ESP_OK;
    }

    return i2s_channel_write(handle->i2s_chan, data, size, bytes_written,
                             timeout_ms);
}

esp_err_t max98357a_set_volume(max98357a_handle_t handle, uint8_t volume)
{
    if (!handle || !handle->is_initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    if (volume > 100) {
        volume = 100;
    }
    handle->volume = volume;

    ESP_LOGI(TAG, "Volume set to %d%%", volume);
    return ESP_OK;
}

esp_err_t max98357a_set_mute(max98357a_handle_t handle, bool mute)
{
    if (!handle || !handle->is_initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    if (handle->sd_mode_io_num >= 0) {
        gpio_set_level(handle->sd_mode_io_num, !mute);
    }

    handle->is_muted = mute;
    ESP_LOGI(TAG, "Mute %s", mute ? "enabled" : "disabled");
    return ESP_OK;
}

esp_err_t max98357a_get_i2s_chan(max98357a_handle_t handle,
                                 i2s_chan_handle_t *chan_handle)
{
    if (!handle || !handle->is_initialized || !chan_handle) {
        return ESP_ERR_INVALID_ARG;
    }

    *chan_handle = handle->i2s_chan;
    return ESP_OK;
}

esp_err_t max98357a_deinit(max98357a_handle_t handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    if (handle->is_initialized) {
        // 禁用I2S通道
        i2s_channel_disable(handle->i2s_chan);
        // 删除I2S通道
        i2s_del_channel(handle->i2s_chan);

        // 复位SD_MODE引脚
        if (handle->sd_mode_io_num >= 0) {
            gpio_set_level(handle->sd_mode_io_num, 0);
            gpio_reset_pin(handle->sd_mode_io_num);
        }

        handle->is_initialized = false;
    }

    free(handle);
    ESP_LOGI(TAG, "MAX98357A deinitialized");
    return ESP_OK;
}
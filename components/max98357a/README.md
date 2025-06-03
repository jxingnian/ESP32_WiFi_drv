<!--
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-06-02 10:06:59
 * @LastEditors: 星年 && j_xingnian@163.com
 * @LastEditTime: 2025-06-02 10:07:29
 * @FilePath: \hello_world\components\max98357a\README.md
 * @Description: 
 * 
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved. 
-->

// 播放示例
void playback_task(void *arg)
{
    // 配置MAX98357A
    max98357a_config_t max_config = {
        .i2s_port = I2S_NUM_1,
        .bck_io_num = 26,
        .ws_io_num = 25,
        .data_io_num = 22,
        .sd_mode_io_num = 21,
        .sample_rate = 44100,
        .bits_per_sample = 16
    };

    max98357a_handle_t max_dev = NULL;
    ESP_ERROR_CHECK(max98357a_init(&max_config, &max_dev));

    // 设置音量和取消静音
    max98357a_set_volume(max_dev, 80);  // 80%音量
    max98357a_set_mute(max_dev, false);

    // 分配缓冲区
    size_t buf_size = 1024;
    uint8_t *buffer = malloc(buf_size);
    size_t bytes_written;

    // 写入音频数据
    while (1) {
        // 获取要播放的数据...
        esp_err_t ret = max98357a_write(max_dev, buffer, buf_size, &bytes_written, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write audio data");
        }
    }

    // 清理
    free(buffer);
    max98357a_deinit(max_dev);
}
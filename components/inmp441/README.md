<!--
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-06-02 10:07:01
 * @LastEditors: 星年 && j_xingnian@163.com
 * @LastEditTime: 2025-06-02 10:07:08
 * @FilePath: \hello_world\components\inmp441\README.md
 * @Description: 
 * 
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved. 
-->
// 录音示例
void record_task(void *arg)
{
    // 配置INMP441
    inmp441_config_t inmp_config = {
        .i2s_port = I2S_NUM_0,
        .sck_io_num = 14,
        .ws_io_num = 15,
        .sd_io_num = 16,
        .sample_rate = 44100,
        .bits_per_sample = 16
    };

    inmp441_handle_t inmp_dev = NULL;
    ESP_ERROR_CHECK(inmp441_init(&inmp_config, &inmp_dev));

    // 分配缓冲区
    size_t buf_size = 1024;
    uint8_t *buffer = malloc(buf_size);
    size_t bytes_read;

    // 读取音频数据
    while (1) {
        esp_err_t ret = inmp441_read(inmp_dev, buffer, buf_size, &bytes_read, portMAX_DELAY);
        if (ret == ESP_OK && bytes_read > 0) {
            // 处理音频数据...
        }
    }

    // 清理
    free(buffer);
    inmp441_deinit(inmp_dev);
}

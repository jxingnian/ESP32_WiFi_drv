# WiFi 驱动组件说明

本组件封装了 ESP32 WiFi 常用操作，接口简洁，便于集成到各类项目中。

## 提供的主要接口

- `wifi_drv_init()`：初始化 WiFi 驱动（需先初始化 NVS）。
- `wifi_drv_deinit()`：反初始化 WiFi 驱动。
- `wifi_drv_connect(ssid, password)`：连接指定 WiFi（STA模式）。
- `wifi_drv_disconnect()`：断开当前 WiFi 连接。
- `wifi_drv_scan(&ap_list, &ap_num)`：扫描周围 WiFi，返回AP列表（需手动free）。
- `wifi_drv_set_mode(mode)`：切换 WiFi 工作模式（STA/AP/STA+AP/NULL）。
- `wifi_drv_save_ap(&ap_info)`：保存 WiFi 信息到 NVS。
- `wifi_drv_get_ap(&ap_info)`：从 NVS 获取已保存的 WiFi 信息。
- `wifi_drv_erase_ap()`：删除 NVS 中保存的 WiFi 信息。

## 使用说明

1. **初始化**
   ```c
   nvs_flash_init();
   wifi_drv_init();
   ```

2. **连接 WiFi**
   ```c
   wifi_drv_connect("your_ssid", "your_password");
   ```

3. **扫描 WiFi**
   ```c
   wifi_ap_record_t *ap_list = NULL;
   uint16_t ap_num = 0;
   wifi_drv_scan(&ap_list, &ap_num);
   // 遍历 ap_list[0..ap_num-1]
   free(ap_list);
   ```

4. **保存/获取/删除 WiFi 配置**
   ```c
   wifi_ap_info_t info = { .ssid = "xxx", .password = "yyy" };
   wifi_drv_save_ap(&info);
   wifi_drv_get_ap(&info);
   wifi_drv_erase_ap();
   ```

5. **反初始化**
   ```c
   wifi_drv_deinit();
   ```

## 注意事项

- 使用前需先初始化 NVS（`nvs_flash_init()`）。
- 扫描结果需手动释放内存。
- NVS 只保存一个 WiFi 配置，如需多组请自行扩展。
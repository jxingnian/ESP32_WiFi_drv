#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// 启动网页配网（挂载SPIFFS、启动AP、HTTP服务器）
void web_prov_start(void);

// 关闭网页配网
void web_prov_stop(void);

#ifdef __cplusplus
}
#endif
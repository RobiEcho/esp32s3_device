#ifndef __WIFI_H__
#define __WIFI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_wifi.h"
#include "esp_event.h"

typedef void (*wifi_conn_callback_t)(void);

/**
 * @brief WiFi事件处理函数
 */
void wifi_event_handler(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data);

/**
 * @brief  初始化WiFi连接模块
 */
void wifi_init(wifi_conn_callback_t callback);


/**
 * @brief 初始化并启动 WiFi STA 模式
 */
void wifi_init_sta(void);

#ifdef __cplusplus
}
#endif

#endif
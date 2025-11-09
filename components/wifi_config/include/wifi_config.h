#ifndef __WIFI_H__
#define __WIFI_H__

#include "esp_wifi.h"
#include "esp_event.h"

/* WiFi连接配置 */
#ifndef WIFI_SSID
#define WIFI_SSID      "80777"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS      "be663398@"
#endif

// WiFi连接成功回调函数类型
typedef void (*wifi_conn_callback_t)(void);

/**
 * @brief 初始化WiFi STA模式并开始连接
 * @param callback 连接成功时的回调函数（可为NULL）
 */
void wifi_init_sta(wifi_conn_callback_t callback);

/**
 * @brief 查询WiFi连接状态
 * @return true表示已连接，false表示未连接
 */
bool wifi_is_connected(void);

#endif
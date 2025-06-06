#ifndef __WIFI_H__
#define __WIFI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_wifi.h"
#include "esp_event.h"

/* 最大重连次数 */
#define WIFI_MAX_RETRY        5
/* WiFi断开后重连等待时间（毫秒） */
#define WIFI_RECONNECT_DELAY  3000

/**
 * @brief  WiFi连接状态回调函数类型
 * @param  connected true表示连接成功，false表示断开连接
 * @param  ip_addr   成功时包含IP地址字符串，断开时为NULL
 */
typedef void (*wifi_conn_callback_t)(bool connected, const char *ip_addr);

/**
 * @brief  初始化WiFi连接模块
 * @param  ssid      WiFi网络SSID
 * @param  password  WiFi密码
 * @param  callback  连接状态回调函数（可为NULL）
 * @return esp_err_t 错误代码
 */
esp_err_t wifi_init(const char *ssid, const char *password, wifi_conn_callback_t callback);

/**
 * @brief  获取当前连接状态
 * @return true 已连接，false 未连接
 */
bool wifi_conn_is_connected(void);

/**
 * @brief  手动触发重新连接
 */
void wifi_conn_retry(void);

#ifdef __cplusplus
}
#endif

#endif //__WIFI_H__
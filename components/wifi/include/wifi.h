#ifndef __WIFI_H__
#define __WIFI_H__

#include "esp_err.h"
#include "wifi_config.h"

/**
 * @brief WiFi 连接状态枚举
 */
typedef enum {
    WIFI_STATE_DISCONNECTED = 0,    // 未连接
    WIFI_STATE_CONNECTING = 1,      // 连接中
    WIFI_STATE_CONNECTED = 2,       // 已连接
    WIFI_STATE_FAILED = 3,          // 连接失败（达到最大重连次数）
} wifi_state_t;

/**
 * @brief 启动 WiFi（STA 或 AP 模式，由 wifi_config.h 配置）
 */
esp_err_t wifi_start(void);

/**
 * @brief 断开 WiFi（STA: 禁用自动重连，AP: 停止热点）
 */
esp_err_t wifi_disconnect(void);

#if (WIFI_APP_MODE == WIFI_APP_MODE_STA)
/**
 * @brief 重新连接 WiFi 并启用自动重连（仅 STA 模式）
 */
esp_err_t wifi_connect(void);
#endif

/**
 * @brief 获取当前 WiFi 连接状态
 */
wifi_state_t wifi_get_state(void);

#endif

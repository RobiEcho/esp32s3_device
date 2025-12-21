#ifndef __WIFI_H__
#define __WIFI_H__

#include <stdbool.h>
#include "esp_err.h"
#include "wifi_config.h"

/**
 * @brief 初始化 WiFi 子系统（NVS / netif / event loop / esp_wifi_init）
 */
esp_err_t wifi_init(void);

bool wifi_is_inited(void);

/**
 * @brief 按 wifi_config.h 的编译期配置启动 WiFi（STA 或 AP）
 */
esp_err_t wifi_start(void);

esp_err_t wifi_disconnect(void);

/**
 * @brief 查询WiFi连接状态
 * @return true表示已连接，false表示未连接
 */
bool wifi_is_connected(void);

#endif
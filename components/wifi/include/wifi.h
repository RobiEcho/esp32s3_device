#ifndef __WIFI_H__
#define __WIFI_H__

#include <stdbool.h>
#include "esp_err.h"
#include "wifi_config.h"

/**
 * @brief 按 wifi_config.h 的编译期配置启动 WiFi（STA 或 AP）
 */
esp_err_t wifi_start(void);

esp_err_t wifi_disconnect(void);

#endif
#ifndef __WIFI_CONFIG_H__
#define __WIFI_CONFIG_H__

#include "esp_wifi_types.h"

#define WIFI_APP_MODE_STA   1
#define WIFI_APP_MODE_AP    2

#ifndef WIFI_APP_MODE
#define WIFI_APP_MODE WIFI_APP_MODE_STA
#endif

/* ==================== STA Config ==================== */
#define WIFI_STA_SSID          "DragonG"
#define WIFI_STA_PASS          "lrt13729011089"
#define WIFI_STA_AUTHMODE      WIFI_AUTH_WPA2_PSK

/* ==================== AP Config ===================== */
#define WIFI_AP_SSID           "ESP32S3_AP"
#define WIFI_AP_PASS           "12345678"
#define WIFI_AP_AUTHMODE       WIFI_AUTH_WPA_WPA2_PSK
#define WIFI_AP_CHANNEL        1
#define WIFI_AP_MAX_CONN       4

/* ================= Reconnect Config ================= */
#define WIFI_RECONNECT_MAX_ATTEMPTS     5           // 最大重连次数
#define WIFI_RECONNECT_BASE_DELAY_MS    1000        // 基础延迟 1 秒
#define WIFI_RECONNECT_MAX_DELAY_MS     30000       // 最大延迟 30 秒

#endif /* __WIFI_CONFIG_H__ */

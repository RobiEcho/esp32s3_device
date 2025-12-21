#include "wifi.h"
#include "wifi_config.h"

#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include <string.h>

static const char *TAG = "WiFi";

static bool s_inited = false;
static volatile bool s_connected = false;
static bool s_netif_created = false;

static esp_netif_t *s_wifi_netif = NULL;

static void _wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;

#if (WIFI_APP_MODE == WIFI_APP_MODE_STA)
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            ESP_LOGI(TAG, "WiFi STA 模式启动");
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            s_connected = false;
            ESP_LOGW(TAG, "WiFi 断开，尝试重新连接...");
            esp_wifi_connect();
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "WiFi 连接成功，IP: " IPSTR, IP2STR(&event->ip_info.ip));
            s_connected = true;
        }
    }
#elif (WIFI_APP_MODE == WIFI_APP_MODE_AP)
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_AP_START) {
            ESP_LOGI(TAG, "WiFi AP 启动");
            s_connected = true;

            if (s_wifi_netif != NULL) {
                esp_netif_ip_info_t ip_info;
                if (esp_netif_get_ip_info(s_wifi_netif, &ip_info) == ESP_OK) {
                    ESP_LOGI(TAG, "WiFi AP IP: " IPSTR, IP2STR(&ip_info.ip));
                }
            }
        } else if (event_id == WIFI_EVENT_AP_STOP) {
            ESP_LOGI(TAG, "WiFi AP 停止");
            s_connected = false;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_AP_STAIPASSIGNED) {
            ip_event_ap_staipassigned_t *event = (ip_event_ap_staipassigned_t *)event_data;
            ESP_LOGI(TAG, "STA got IP: " IPSTR, IP2STR(&event->ip));
        }
    }
#endif
}

esp_err_t wifi_init(void)
{
    if (s_inited) {
        return ESP_OK;
    }

    // 1) 初始化 NVS（WiFi/系统组件可能依赖）
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        err = nvs_flash_erase();
        if (err != ESP_OK) {
            return err;
        }
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        return err;
    }

    // 2) 初始化网络接口层（esp_netif）
    err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    // 3) 创建默认事件循环（用于派发 WIFI_EVENT/IP_EVENT）
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    // 4) 创建默认 WiFi 网卡对象（STA 或 AP，按编译期配置）
    if (!s_netif_created) {
#if (WIFI_APP_MODE == WIFI_APP_MODE_STA)
        s_wifi_netif = esp_netif_create_default_wifi_sta();
#elif (WIFI_APP_MODE == WIFI_APP_MODE_AP)
        s_wifi_netif = esp_netif_create_default_wifi_ap();
#else
        return ESP_ERR_INVALID_ARG;
#endif
        s_netif_created = true;
    }

    // 5) 初始化 WiFi Driver
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        return err;
    }

    // 6) 注册事件处理函数（处理连接/断开/拿到 IP 等）
    err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, _wifi_event_handler, NULL);
    if (err != ESP_OK) {
        return err;
    }

#if (WIFI_APP_MODE == WIFI_APP_MODE_STA)
    err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, _wifi_event_handler, NULL);
    if (err != ESP_OK) {
        return err;
    }
#elif (WIFI_APP_MODE == WIFI_APP_MODE_AP)
    err = esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, _wifi_event_handler, NULL);
    if (err != ESP_OK) {
        return err;
    }
#endif

    s_inited = true;
    ESP_LOGI(TAG, "inited");
    return ESP_OK;
}

bool wifi_is_inited(void)
{
    return s_inited;
}

#if (WIFI_APP_MODE == WIFI_APP_MODE_STA)
static esp_err_t _wifi_connect_sta(const char *ssid, const char *pass)
{
    if (ssid == NULL || ssid[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_inited) {
        return ESP_ERR_INVALID_STATE;
    }

    wifi_config_t wifi_config = { 0 };
    (void)strlcpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    if (pass != NULL) {
        (void)strlcpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
    }

    wifi_config.sta.threshold.authmode = WIFI_STA_AUTHMODE;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_wifi_start();
    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
        return err;
    }

    ESP_LOGI(TAG, "sta start, connecting...");
    return ESP_OK;
}
#endif

#if (WIFI_APP_MODE == WIFI_APP_MODE_AP)
static esp_err_t _wifi_start_ap(void)
{
    if (!s_inited) {
        return ESP_ERR_INVALID_STATE;
    }

    wifi_config_t wifi_config = { 0 };

    (void)strlcpy((char *)wifi_config.ap.ssid, WIFI_AP_SSID, sizeof(wifi_config.ap.ssid));
    wifi_config.ap.ssid_len = (uint8_t)strlen(WIFI_AP_SSID);

    (void)strlcpy((char *)wifi_config.ap.password, WIFI_AP_PASS, sizeof(wifi_config.ap.password));
    wifi_config.ap.channel = WIFI_AP_CHANNEL;
    wifi_config.ap.max_connection = WIFI_AP_MAX_CONN;

    if (WIFI_AP_PASS[0] == '\0') {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    } else {
        wifi_config.ap.authmode = WIFI_AP_AUTHMODE;
    }

    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) {
        return err;
    }

    err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK) {
        return err;
    }

    err = esp_wifi_start();
    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
        return err;
    }

    ESP_LOGI(TAG, "ap start");
    return ESP_OK;
}
#endif

esp_err_t wifi_start(void)
{
    if (!s_inited) {
        esp_err_t err = wifi_init();
        if (err != ESP_OK) {
            return err;
        }
    }

#if (WIFI_APP_MODE == WIFI_APP_MODE_STA)
    return _wifi_connect_sta(WIFI_STA_SSID, WIFI_STA_PASS);
#elif (WIFI_APP_MODE == WIFI_APP_MODE_AP)
    return _wifi_start_ap();
#else
    return ESP_ERR_INVALID_ARG;
#endif
}

esp_err_t wifi_disconnect(void)
{
    if (!s_inited) {
        return ESP_ERR_INVALID_STATE;
    }
    s_connected = false;
#if (WIFI_APP_MODE == WIFI_APP_MODE_STA)
    return esp_wifi_disconnect();
#elif (WIFI_APP_MODE == WIFI_APP_MODE_AP)
    return esp_wifi_stop();
#else
    return ESP_ERR_INVALID_ARG;
#endif
}

bool wifi_is_connected(void)
{
    return s_connected;
}
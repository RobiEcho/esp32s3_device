#include "wifi.h"
#include "wifi_credentials.h"
#include "nvs_storage.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <string.h>

extern bool g_nvs_initialized;

static const char *TAG = "wifi_app";

static bool s_inited = false;
static esp_netif_t *s_wifi_netif = NULL;
static wifi_state_t s_wifi_state = WIFI_STATE_DISCONNECTED;  // wifi连接状态

#if (WIFI_APP_MODE == WIFI_APP_MODE_STA)
// 重连（仅 STA 模式）
static uint8_t s_reconnect_attempts = 0;                     // 重连次数
static TimerHandle_t s_reconnect_timer = NULL;               // 重连定时器句柄
static bool s_auto_reconnect = true;                         // 是否自动重连标志

static void _reconnect_timer_callback(TimerHandle_t xTimer)
{
    (void)xTimer;
    
    if (s_reconnect_attempts < WIFI_RECONNECT_MAX_ATTEMPTS) {
        s_reconnect_attempts++;
        ESP_LOGI(TAG, "重连 %d/%d", s_reconnect_attempts, WIFI_RECONNECT_MAX_ATTEMPTS);
        esp_wifi_connect();
    } else {
        ESP_LOGE(TAG, "重连失败，已达最大次数");
        s_wifi_state = WIFI_STATE_FAILED;
        if (s_reconnect_timer != NULL) {
            xTimerDelete(s_reconnect_timer, 0);
            s_reconnect_timer = NULL;
        }
    }
}

static void _start_reconnect_timer(void)
{
    if (s_reconnect_timer != NULL) {
        xTimerDelete(s_reconnect_timer, 0);
    }
    
    // 指数退避：1s, 2s, 4s, 8s, 16s, 30s(上限)
    uint32_t delay_ms = WIFI_RECONNECT_BASE_DELAY_MS * (1 << s_reconnect_attempts);
    if (delay_ms > WIFI_RECONNECT_MAX_DELAY_MS) {
        delay_ms = WIFI_RECONNECT_MAX_DELAY_MS;
    }
    
    ESP_LOGI(TAG, "延迟 %lu ms 后重连", delay_ms);
    
    s_reconnect_timer = xTimerCreate(
        "wifi_reconnect",
        pdMS_TO_TICKS(delay_ms),
        pdFALSE,
        NULL,
        _reconnect_timer_callback
    );
    
    if (s_reconnect_timer != NULL) {
        xTimerStart(s_reconnect_timer, 0);
    }
}

static void _reset_reconnect_state(void)
{
    s_reconnect_attempts = 0;
    if (s_reconnect_timer != NULL) {
        xTimerDelete(s_reconnect_timer, 0);
        s_reconnect_timer = NULL;
    }
}
#endif

static void _wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;

#if (WIFI_APP_MODE == WIFI_APP_MODE_STA)
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            ESP_LOGI(TAG, "STA 启动");
            s_wifi_state = WIFI_STATE_CONNECTING;
            _reset_reconnect_state();
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGW(TAG, "连接断开");
            s_wifi_state = WIFI_STATE_DISCONNECTED;
            
            // 只有在允许自动重连时才启动重连定时器
            if (s_auto_reconnect && s_reconnect_attempts < WIFI_RECONNECT_MAX_ATTEMPTS) {
                _start_reconnect_timer();
            } else if (!s_auto_reconnect) {
                ESP_LOGI(TAG, "自动重连已禁用");
            } else {
                ESP_LOGE(TAG, "重连失败，已达最大次数");
                s_wifi_state = WIFI_STATE_FAILED;
            }
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "连接成功，IP: " IPSTR, IP2STR(&event->ip_info.ip));
            s_wifi_state = WIFI_STATE_CONNECTED;
            _reset_reconnect_state();
        }
    }
#elif (WIFI_APP_MODE == WIFI_APP_MODE_AP)
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_AP_START) {
            ESP_LOGI(TAG, "AP 启动");
            s_wifi_state = WIFI_STATE_CONNECTED;

            if (s_wifi_netif != NULL) {
                esp_netif_ip_info_t ip_info;
                if (esp_netif_get_ip_info(s_wifi_netif, &ip_info) == ESP_OK) {
                    ESP_LOGI(TAG, "AP IP: " IPSTR, IP2STR(&ip_info.ip));
                }
            }
        } else if (event_id == WIFI_EVENT_AP_STOP) {
            ESP_LOGI(TAG, "AP 停止");
            s_wifi_state = WIFI_STATE_DISCONNECTED;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_AP_STAIPASSIGNED) {
            ip_event_ap_staipassigned_t *event = (ip_event_ap_staipassigned_t *)event_data;
            ESP_LOGI(TAG, "客户端 IP: " IPSTR, IP2STR(&event->ip));
        }
    }
#endif
}

static esp_err_t _wifi_init(void)
{
    if (s_inited) {
        return ESP_OK;
    }

    // 检查 NVS 是否已初始化，如果未初始化则初始化
    if (!g_nvs_initialized) {
        esp_err_t err = nvs_storage_init();
        if (err != ESP_OK) {
            return err;
        }
    }

    // 初始化网络接口层（esp_netif）
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    // 创建默认事件循环（用于派发 WIFI_EVENT/IP_EVENT）
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    // 创建默认 wifi 网卡对象（STA 或 AP）
    if (s_wifi_netif == NULL) {
#if (WIFI_APP_MODE == WIFI_APP_MODE_STA)
        s_wifi_netif = esp_netif_create_default_wifi_sta();
#elif (WIFI_APP_MODE == WIFI_APP_MODE_AP)
        s_wifi_netif = esp_netif_create_default_wifi_ap();
#endif
    }

    // 初始化 wifi Driver
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    // 注册 WiFi 事件处理函数（处理连接/断开等）
    err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, _wifi_event_handler, NULL);
    if (err != ESP_OK) {
        return err;
    }

    // 注册 IP 事件处理函数（处理获取 IP 等）
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
    ESP_LOGI(TAG, "WiFi 初始化完成");
    return ESP_OK;
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

    return ESP_OK;
}
#elif (WIFI_APP_MODE == WIFI_APP_MODE_AP)
static esp_err_t _wifi_start_ap(const char *ssid, const char *password)
{
    if (!s_inited) {
        return ESP_ERR_INVALID_STATE;
    }

    if (ssid == NULL || ssid[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    wifi_config_t wifi_config = { 0 };

    (void)strlcpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
    wifi_config.ap.ssid_len = (uint8_t)strlen(ssid);

    if (password != NULL) {
        (void)strlcpy((char *)wifi_config.ap.password, password, sizeof(wifi_config.ap.password));
    }
    
    wifi_config.ap.channel = WIFI_AP_CHANNEL;
    wifi_config.ap.max_connection = WIFI_AP_MAX_CONN;

    if (password == NULL || password[0] == '\0') {
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

    return ESP_OK; 
}
#endif

esp_err_t wifi_start(void)
{
    if (!s_inited) {
        esp_err_t err = _wifi_init();
        if (err != ESP_OK) {
            return err;
        }
    }

#if (WIFI_APP_MODE == WIFI_APP_MODE_STA)
    char ssid[32] = {0};
    char password[64] = {0};
    
    // 优先从 NVS 读取凭证
    esp_err_t err = wifi_credentials_load_sta(ssid, sizeof(ssid), password, sizeof(password));
    
    // 如果 NVS 中没有凭证，使用编译期默认配置
    if (err != ESP_OK || ssid[0] == '\0') {
        ESP_LOGW(TAG, "NVS 中未找到 STA 凭证，使用默认配置");
        strlcpy(ssid, WIFI_STA_SSID, sizeof(ssid));
        strlcpy(password, WIFI_STA_PASS, sizeof(password));
    }
    
    return _wifi_connect_sta(ssid, password);
#elif (WIFI_APP_MODE == WIFI_APP_MODE_AP)
    char ap_ssid[32] = {0};
    char ap_password[64] = {0};
    
    // 优先从 NVS 读取 AP 凭证
    esp_err_t err = wifi_credentials_load_ap(ap_ssid, sizeof(ap_ssid), ap_password, sizeof(ap_password));
    
    // 如果 NVS 中没有凭证，使用编译期默认配置
    if (err != ESP_OK || ap_ssid[0] == '\0') {
        ESP_LOGW(TAG, "NVS 中未找到 AP 凭证，使用默认配置");
        strlcpy(ap_ssid, WIFI_AP_SSID, sizeof(ap_ssid));
        strlcpy(ap_password, WIFI_AP_PASS, sizeof(ap_password));
    }
    
    return _wifi_start_ap(ap_ssid, ap_password);
#endif
}

esp_err_t wifi_disconnect(void)
{
    if (!s_inited) {
        return ESP_ERR_INVALID_STATE;
    }
#if (WIFI_APP_MODE == WIFI_APP_MODE_STA)
    // 主动断开，不再自动重连
    s_auto_reconnect = false;
    _reset_reconnect_state();
    return esp_wifi_disconnect();
#elif (WIFI_APP_MODE == WIFI_APP_MODE_AP)
    return esp_wifi_stop();
#endif
}

#if (WIFI_APP_MODE == WIFI_APP_MODE_STA)
esp_err_t wifi_connect(void)
{
    if (!s_inited) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 需要重新连接时，启用自动重连
    s_auto_reconnect = true;
    _reset_reconnect_state();
    return esp_wifi_connect();
}
#endif

wifi_state_t wifi_get_state(void)
{
    return s_wifi_state;
}

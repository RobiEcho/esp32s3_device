#include "wifi_config.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "WiFi";

static wifi_conn_callback_t user_callback = NULL;     // 用户注册的回调函数
static volatile bool wifi_connected = false;          // WiFi连接状态

// WiFi事件处理函数
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            // WiFi驱动启动完成，发起首次连接
            ESP_LOGI(TAG, "WiFi STA 模式启动");
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            // WiFi断开，立即重连
            wifi_connected = false;
            ESP_LOGW(TAG, "WiFi 断开，尝试重新连接...");
            esp_wifi_connect();
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            // 成功获取IP地址
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "WiFi 连接成功，IP: " IPSTR, IP2STR(&event->ip_info.ip));
            wifi_connected = true;
            
            // 调用用户注册的回调函数
            if (user_callback) {
                user_callback();
            }
        }
    }
}

// 初始化WiFi STA模式
void wifi_init_sta(wifi_conn_callback_t callback) {
    user_callback = callback;
    
    // 初始化NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // 初始化TCP/IP协议栈和事件循环
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // 初始化WiFi驱动
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 注册事件处理器
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL));
    
    // 配置WiFi连接参数
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        }
    };
    
    // 设置为STA模式并启动
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi STA 模式初始化完成");
}

// 查询WiFi连接状态
bool wifi_is_connected(void) {
    return wifi_connected;
}
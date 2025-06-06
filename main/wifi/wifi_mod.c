#include "wifi_mod.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"

static const char *TAG = "WiFi_STA";

static wifi_conn_callback_t s_user_callback = NULL;

// Wi-Fi 事件处理函数
void wifi_event_handler(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {  // 如果是 Wi-Fi 事件
        if (event_id == WIFI_EVENT_STA_START) {   // Wi-Fi 站点启动事件
            ESP_LOGI(TAG, "STA 模式启动");
            esp_wifi_connect();   // 连接 Wi-Fi
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {   // Wi-Fi 断开连接事件
            ESP_LOGI(TAG, "已断开 Wi-Fi 连接，正在尝试重新连接...");
            esp_wifi_connect();   // 尝试重新连接 Wi-Fi
        }
    } else if (event_base == IP_EVENT) {   // 如果是 IP 事件
        if (event_id == IP_EVENT_STA_GOT_IP) {   // 获取到 IP 地址事件
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;  // 获取 IP 信息
            ESP_LOGI(TAG, "Wifi连接，已获取到 IP 地址: " IPSTR, IP2STR(&event->ip_info.ip));  // 打印 IP 地址
        }
        if (s_user_callback) { // 调用回调函数
            s_user_callback();
        }
    }
}

// STA 模式初始化
void wifi_init(wifi_conn_callback_t callback) {
    s_user_callback = callback;
    // 初始化 NVS
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 仅创建 STA 的网络接口
    esp_netif_create_default_wifi_sta();

    // wifi 配置
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 注册事件处理器
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL));
    
    // 配置 STA 参数
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        }
    };
    // 设置模式为仅 STA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "WIFI_STA 模式初始化完成");
}
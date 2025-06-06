#include "wifi.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "WiFi";
static EventGroupHandle_t s_wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;
static wifi_conn_callback_t user_callback = NULL;
static char configured_ssid[32] = {0};
static char configured_pass[64] = {0};
static uint8_t retry_count = 0;

/* 内部事件处理函数 */
static void event_handler(void* arg, esp_event_base_t event_base, 
                        int32_t event_id, void* event_data)
{
    // WiFi连接成功事件
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "成功连接到AP");
    }
    // 获取IP事件
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "获取到IP地址:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        retry_count = 0; // 重置重试计数器
        
        if(user_callback) {
            char ip_str[16];
            snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&event->ip_info.ip));
            user_callback(true, ip_str);
        }
    }
    // 断开连接事件
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry_count < WIFI_MAX_RETRY) {
            ESP_LOGW(TAG, "连接断开，尝试重连... (%d/%d)", ++retry_count, WIFI_MAX_RETRY);
            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG, "超过最大重试次数");
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            
            if(user_callback) {
                user_callback(false, NULL);
            }
        }
    }
}

esp_err_t wifi_init(const char *ssid, const char *password, wifi_conn_callback_t callback)
{
    // 参数检查
    if(!ssid || !password || strlen(ssid) == 0) {
        ESP_LOGE(TAG, "无效的SSID或密码");
        return ESP_ERR_INVALID_ARG;
    }

    // 保存配置
    strncpy(configured_ssid, ssid, sizeof(configured_ssid)-1);
    strncpy(configured_pass, password, sizeof(configured_pass)-1);
    user_callback = callback;

    // 初始化NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 创建事件组和循环
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // 初始化网络接口
    esp_netif_create_default_wifi_sta();

    // WiFi配置
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // 注册事件处理
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    // 设置STA模式
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    memcpy(wifi_config.sta.ssid, configured_ssid, strlen(configured_ssid));
    memcpy(wifi_config.sta.password, configured_pass, strlen(configured_pass));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "启动WiFi连接至SSID:%s", configured_ssid);
    esp_wifi_connect();

    return ESP_OK;
}

bool wifi_conn_is_connected(void) {
    return (xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT) != 0;
}

void wifi_conn_retry(void) {
    retry_count = 0;
    esp_wifi_connect();
}
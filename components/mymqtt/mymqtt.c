#include "mymqtt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "st7789.h"

static uint8_t img_buf[IMG_BUF_SIZE];
static size_t img_buf_len = 0;

static esp_mqtt_client_handle_t hmqtt = NULL;
static mqtt_conn_callback_t conn_callback = NULL;
static const char *TAG = "MQTT";

static void event_handler(void *args, esp_event_base_t base, int32_t event_id, void *event_data);

void mqtt_init(mqtt_conn_callback_t conn_cb) {
    // 配置MQTT客户端
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_URI,
        .credentials.client_id = MQTT_CLIENT_ID,
        .buffer.size = MQTT_RX_BUFFER_SIZE,
        .network.disable_auto_reconnect = false
    };

    hmqtt = esp_mqtt_client_init(&mqtt_cfg);
    conn_callback = conn_cb;

    // 注册事件回调函数：当MQTT发生事件（如连接、断开、收消息）时，自动调用event_handler
    esp_mqtt_client_register_event(hmqtt, ESP_EVENT_ANY_ID, event_handler, NULL);
    // 启动MQTT客户端（开始连接服务器）
    esp_mqtt_client_start(hmqtt);
}

int mqtt_publish(const char *topic, const void *data, size_t len, int qos) {
    if (!hmqtt || !topic || !data) return -1;
    return esp_mqtt_client_publish(hmqtt, topic, data, len, qos, 0);
}

esp_err_t mqtt_subscribe(const char *topic, int qos) {
    if (!hmqtt || !topic) return ESP_FAIL;
    return esp_mqtt_client_subscribe(hmqtt, topic, qos);
}

static void event_handler(void *args, esp_event_base_t base, int32_t event_id, void *event_data) 
{
    esp_mqtt_event_handle_t event = event_data;
    
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:// 连接成功事件
        ESP_LOGI(TAG, "MQTT 连接成功！");
        if (conn_callback) conn_callback(true);// 调用回调函数        
        // 自动订阅图像主题
        mqtt_subscribe(IMG_TOPIC, 1);
        break;

    case MQTT_EVENT_DISCONNECTED:// 断开连接事件
        ESP_LOGI(TAG, "MQTT 断开连接！");
        if (conn_callback) conn_callback(false);// 调用回调函数
        break;

    case MQTT_EVENT_DATA:// 收到消息事件
        if (strncmp(event->topic, IMG_TOPIC, event->topic_len) == 0) {
            // ESP_LOGI(TAG, "收到数据，长度: %d 字节", event->data_len);
            // 检查是否会溢出
            if (img_buf_len + event->data_len <= IMG_BUF_SIZE) {
                memcpy(img_buf + img_buf_len, event->data, event->data_len);
                img_buf_len += event->data_len;
            } else {
                ESP_LOGW(TAG, "图像数据溢出，已重置缓冲区");
                img_buf_len = 0; // 溢出时丢弃本次数据
            }
            // 判断是否收满一帧
            if (img_buf_len == IMG_BUF_SIZE) {
                ESP_LOGI(TAG, "收到完整图像，总长度: %d 字节", img_buf_len);
                st7789_display_raw(img_buf, IMG_BUF_SIZE);
                img_buf_len = 0; // 重置缓冲区
            }
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT 发生错误，错误代码: %d", event->error_handle->error_type);
        break;

    default:
        break;
    }
}
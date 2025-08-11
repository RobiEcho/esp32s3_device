#include "mymqtt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static esp_mqtt_client_handle_t client = NULL;
static mqtt_conn_callback_t conn_callback = NULL;
static const char *TAG = "MQTT";

static void event_handler(void *args, esp_event_base_t base, int32_t event_id, void *event_data);

void mqtt_comm_init(mqtt_conn_callback_t conn_cb) {
    // 配置MQTT客户端
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_URI,
        .credentials.client_id = MQTT_CLIENT_ID,
        .buffer.size = MQTT_RX_BUFFER_SIZE,
        .network.disable_auto_reconnect = false
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    conn_callback = conn_cb;

    // 注册事件回调
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, event_handler, NULL);
    esp_mqtt_client_start(client);
}

static void event_handler(void *args, esp_event_base_t base, int32_t event_id, void *event_data) 
{
    esp_mqtt_event_handle_t event = event_data;
    
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Connected to MQTT broker");
        if (conn_callback) conn_callback(true);// 调用回调函数        
        // 自动订阅图像主题
        esp_mqtt_client_subscribe(client, IMG_TOPIC, 1);
        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Disconnected from MQTT broker");
        break;
        
    case MQTT_EVENT_DATA:
        if (strncmp(event->topic, IMG_TOPIC, event->topic_len) == 0) {
            // 暂时不处理图像数据
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT 发生错误，错误代码: %d", event->error_handle->error_type);
        break;

    default:
        break;
    }
}

int mqtt_publish(const char *topic, const void *data, size_t len, int qos) {
    if (!client || !topic || !data) return -1;
    return esp_mqtt_client_publish(client, topic, data, len, qos, 0);
}

esp_err_t mqtt_subscribe(const char *topic, int qos) {
    if (!client || !topic) return ESP_FAIL;
    return esp_mqtt_client_subscribe(client, topic, qos);
}
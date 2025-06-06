#include "mqtt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../st7789/st7789.h"
#include "config.h"

static esp_mqtt_client_handle_t client = NULL;
static mqtt_conn_callback_t conn_callback = NULL;
static const char *TAG = "MQTT";

static void event_handler(void *args, esp_event_base_t base, 
                        int32_t event_id, void *event_data);

// 图像双缓冲区
static uint8_t img_buffer[2][IMG_WIDTH * IMG_HEIGHT * IMG_PIXEL_SIZE + sizeof(frame_header_t)];
static size_t img_received = 0;
static uint32_t current_frame_id = 0;  // 当前帧ID
static uint32_t expected_frame_len = 0; // 预期帧长度
static int buf_index_recv = 0; // 当前接收缓冲区索引
static int buf_index_disp = 1; // 当前显示缓冲区索引

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

// 处理帧数据，返回本次消耗的数据长度（便于处理粘包）
static size_t process_frame_data(const uint8_t *data, size_t len) {
    size_t consumed = 0;
    if (img_received == 0) {
        if (len < sizeof(frame_header_t)) {
            ESP_LOGW(TAG, "接收数据太小，无法包含帧头");
            return 0;
        }
        frame_header_t *header = (frame_header_t *)data;
        current_frame_id = header->frame_id;
        expected_frame_len = header->frame_len;
        if (expected_frame_len != IMG_WIDTH * IMG_HEIGHT * IMG_PIXEL_SIZE) {
            ESP_LOGE(TAG, "无效的帧长度: %u (预期: %u)", expected_frame_len, IMG_WIDTH * IMG_HEIGHT * IMG_PIXEL_SIZE);
            return sizeof(frame_header_t);
        }
        size_t data_len = len - sizeof(frame_header_t);
        if (data_len > 0) {
            memcpy(img_buffer[buf_index_recv], data + sizeof(frame_header_t), data_len);
            img_received = data_len;
        } else {
            img_received = 0;
        }
        consumed = (data_len > 0) ? len : sizeof(frame_header_t);
    } else {
        size_t remain = expected_frame_len - img_received;
        size_t copy_len = (len < remain) ? len : remain;
        memcpy(img_buffer[buf_index_recv] + img_received, data, copy_len);
        img_received += copy_len;
        consumed = copy_len;
    }
    if (img_received >= expected_frame_len) {
        // 交换缓冲区指针，显示上一帧，接收下一帧
        int tmp = buf_index_recv;
        buf_index_recv = buf_index_disp;
        buf_index_disp = tmp;
        st7789_display_raw(img_buffer[buf_index_disp], expected_frame_len);
        size_t extra_len = img_received - expected_frame_len + (len - consumed);
        if (extra_len > 0) {
            size_t offset = consumed + (img_received - expected_frame_len);
            ESP_LOGW(TAG, "检测到粘包: 额外数据 %u 字节", extra_len);
            if (offset < len) {
                memmove(img_buffer[buf_index_recv], data + offset, len - offset);
                img_received = len - offset;
                consumed += (len - offset);
            } else {
                img_received = 0;
            }
        } else {
            img_received = 0;
        }
        return consumed;
    }
    return consumed;
}

static void event_handler(void *args, esp_event_base_t base, 
                        int32_t event_id, void *event_data) 
{
    esp_mqtt_event_handle_t event = event_data;
    static uint32_t frame_count = 0;
    static uint32_t last_frame_time = 0;
    
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Connected to MQTT broker");
        if (conn_callback) conn_callback(true);// 调用回调函数        
        // 自动订阅图像主题
        esp_mqtt_client_subscribe(client, IMG_TOPIC, 1);
        // 重置图像接收状态
        img_received = 0;
        current_frame_id = 0;
        expected_frame_len = 0;
        frame_count = 0;
        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Disconnected from MQTT broker");
        break;
        
    case MQTT_EVENT_DATA:
        if (strncmp(event->topic, IMG_TOPIC, event->topic_len) == 0) {
            size_t offset = 0;// 当前偏移量
            size_t remain = event->data_len;
            // 循环处理可能的多个粘包帧
            while (remain > 0) {
                size_t consumed = process_frame_data((const uint8_t *)event->data + offset, remain);
                if (consumed == 0) break; // 数据不足，等待下次补齐
                offset += consumed;
                remain -= consumed;
            }
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT 发生错误，错误代码: %d",
                         event->error_handle->error_type);
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
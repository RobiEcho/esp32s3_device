#include "mymqtt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "st7789.h"

static uint8_t img_buf[IMG_BUF_SIZE];              // 图像数据缓冲区（240*240*2=115200字节）
static size_t img_buf_len = 0;                     // 当前已接收的图像数据长度
static int64_t last_img_time = 0;                  // 上次接收图像数据的时间戳（用于超时检测）

static esp_mqtt_client_handle_t hmqtt = NULL;      // MQTT客户端句柄
static mqtt_conn_callback_t conn_callback = NULL;  // 用户注册的连接状态回调函数
static volatile bool mqtt_connected = false;       // MQTT连接状态标志（多任务访问需volatile）
static const char *TAG = "MQTT";                   // 日志标签

static void event_handler(void *args, esp_event_base_t base, int32_t event_id, void *event_data);

void mqtt_init(mqtt_conn_callback_t conn_cb) {
    // MQTT客户端配置
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_URI,               // MQTT代理服务器地址
        .credentials.client_id = MQTT_CLIENT_ID,      // 客户端唯一标识符
        .buffer.size = MQTT_RX_BUFFER_SIZE,           // 接收缓冲区大小
        .network.disable_auto_reconnect = false       // 启用自动重连
    };

    hmqtt = esp_mqtt_client_init(&mqtt_cfg);
    conn_callback = conn_cb;

    // 注册事件回调函数
    esp_mqtt_client_register_event(hmqtt, ESP_EVENT_ANY_ID, event_handler, NULL);
    // 启动MQTT客户端
    esp_mqtt_client_start(hmqtt);
}

int mqtt_publish(const char *topic, const void *data, size_t len, int qos) {
    if (!hmqtt || !topic || !data) {
        ESP_LOGW(TAG, "MQTT发布失败：参数无效");
        return -1;
    }
    
    if (!mqtt_connected) {
        ESP_LOGW(TAG, "MQTT未连接，无法发布数据");
        return -1;
    }
    
    int msg_id = esp_mqtt_client_publish(hmqtt, topic, data, len, qos, 0);
    if (msg_id == -1) {
        ESP_LOGE(TAG, "MQTT发布失败，主题: %s", topic);
    }
    return msg_id;
}

esp_err_t mqtt_subscribe(const char *topic, int qos) {
    if (!hmqtt || !topic) return ESP_FAIL;
    return esp_mqtt_client_subscribe(hmqtt, topic, qos);
}

bool mqtt_is_connected(void) {
    return mqtt_connected;
}

static void event_handler(void *args, esp_event_base_t base, int32_t event_id, void *event_data) 
{
    esp_mqtt_event_handle_t event = event_data;
    
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:// 连接成功事件
        ESP_LOGI(TAG, "MQTT 连接成功！");
        mqtt_connected = true;  // 更新连接状态
        if (conn_callback) conn_callback(true);      
        mqtt_subscribe(IMG_TOPIC, 1);// 订阅图像主题
        break;

    case MQTT_EVENT_DISCONNECTED:// 断开连接事件
        ESP_LOGI(TAG, "MQTT 断开连接！");
        mqtt_connected = false;  // 更新连接状态
        img_buf_len = 0;         // 清理未完成的图像接收
        if (conn_callback) conn_callback(false);// 调用回调函数
        break;

    case MQTT_EVENT_DATA:// 收到消息事件
        if (strncmp(event->topic, IMG_TOPIC, event->topic_len) == 0) {
            int64_t current_time = esp_timer_get_time();
            
            // 超时检测
            if (img_buf_len > 0 && (current_time - last_img_time) > 100000) { // 100ms
                ESP_LOGW(TAG, "图像接收超时，重置缓冲区（已接收 %d/%d 字节）", 
                         img_buf_len, IMG_BUF_SIZE);
                img_buf_len = 0;
            }
            
            last_img_time = current_time;  // 更新时间戳
            
            // 检查是否会溢出
            if (img_buf_len + event->data_len <= IMG_BUF_SIZE) {
                memcpy(img_buf + img_buf_len, event->data, event->data_len);
                img_buf_len += event->data_len;// 更新指针位置
            } else {
                ESP_LOGW(TAG, "图像数据溢出，已重置缓冲区（尝试写入 %d 字节，已有 %d 字节）", 
                         event->data_len, img_buf_len);
                img_buf_len = 0; // 溢出时丢弃本次数据
            }
            
            // 判断是否收满一帧，显示图像
            if (img_buf_len == IMG_BUF_SIZE) {
                ESP_LOGI(TAG, "收到完整图像，总长度: %d 字节", img_buf_len);
                // st7789_display_raw(img_buf, IMG_BUF_SIZE);
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
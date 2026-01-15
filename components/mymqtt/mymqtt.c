#include "mymqtt.h"
#include "mymqtt_config.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>

static const char *TAG = "mymqtt";

static esp_mqtt_client_handle_t s_hmqtt = NULL;
static bool s_inited = false;
static volatile bool s_connected = false;

static mymqtt_image_cb_t s_image_cb = NULL;

static uint8_t *s_img_buf = NULL;           // 图像拼接缓冲区（115200 字节）
static size_t s_img_buf_len = 0;            // 当前已接收字节数
static bool s_receiving_image = false;      // 是否正在接收图像分片

// 处理图像分片数据
static void _mymqtt_handle_image_data(const uint8_t *data, size_t data_len)
{
    if (s_img_buf == NULL) return;

    // 溢出检测
    if (s_img_buf_len + data_len > MYMQTT_IMG_BUF_SIZE) {
        ESP_LOGW(TAG, "图像数据溢出，重置");
        s_img_buf_len = 0;
        return;
    }

    // 拼接数据到缓冲区
    memcpy(s_img_buf + s_img_buf_len, data, data_len);
    s_img_buf_len += data_len;

    // 收满一帧，调用回调绘制
    if (s_img_buf_len == MYMQTT_IMG_BUF_SIZE) {
        ESP_LOGI(TAG, "收到完整图像帧");
        if (s_image_cb) {
            s_image_cb((const uint16_t *)s_img_buf);
        }
        s_img_buf_len = 0;
        s_receiving_image = false;
    }
}

// MQTT 事件处理
static void _mymqtt_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)arg;
    (void)base;
    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "已连接");
        s_connected = true;
        // 自动订阅图像主题
        if (s_image_cb) {
            esp_mqtt_client_subscribe(s_hmqtt, MYMQTT_TOPIC_IMAGE, 1);
        }
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "已断开");
        s_connected = false;
        s_img_buf_len = 0;
        s_receiving_image = false;
        break;

    case MQTT_EVENT_DATA:
        // 第一个分片带主题名，后续分片 topic_len=0
        if (event->topic_len > 0) {
            s_receiving_image = (strncmp(event->topic, MYMQTT_TOPIC_IMAGE, event->topic_len) == 0);
            if (s_receiving_image) {
                s_img_buf_len = 0;  // 新图像，重置缓冲区
            }
        }
        // 正在接收图像，处理分片数据
        if (s_receiving_image && event->data && event->data_len > 0) {
            _mymqtt_handle_image_data((const uint8_t *)event->data, event->data_len);
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "错误: %d", event->error_handle->error_type);
        break;

    default:
        break;
    }
}

esp_err_t mymqtt_init(mymqtt_image_cb_t image_cb)
{
    if (s_inited) {
        return ESP_OK;
    }

    s_image_cb = image_cb;

    // 如果需要接收图像，分配拼接缓冲区
    if (image_cb) {
#if defined(CONFIG_GRAPHICS_USE_PSRAM)
        s_img_buf = heap_caps_malloc(MYMQTT_IMG_BUF_SIZE, MALLOC_CAP_SPIRAM);
#else
        s_img_buf = heap_caps_malloc(MYMQTT_IMG_BUF_SIZE, MALLOC_CAP_DMA);
#endif
        if (s_img_buf == NULL) {
            ESP_LOGE(TAG, "图像缓冲区分配失败");
            return ESP_ERR_NO_MEM;
        }
    }

    // MQTT 客户端配置
    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = MYMQTT_BROKER_URI,                   // 代理服务器地址
        .credentials.client_id = MYMQTT_CLIENT_ID,                 // 客户端ID
        .credentials.username = MYMQTT_USERNAME,                   // 用户名
        .credentials.authentication.password = MYMQTT_PASSWORD,    // 密码
        .buffer.size = MYMQTT_RX_BUFFER_SIZE,                      // 接收缓冲区大小
        .network.disable_auto_reconnect = false,                   // 启用自动重连
    };

    // 创建 MQTT 客户端
    s_hmqtt = esp_mqtt_client_init(&cfg);
    if (s_hmqtt == NULL) {
        ESP_LOGE(TAG, "客户端初始化失败");
        return ESP_FAIL;
    }

    // 注册事件回调
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(s_hmqtt, ESP_EVENT_ANY_ID, _mymqtt_event_handler, NULL));

    // 启动客户端
    ESP_ERROR_CHECK(esp_mqtt_client_start(s_hmqtt));

    s_inited = true;
    ESP_LOGI(TAG, "初始化完成");
    return ESP_OK;
}

bool mymqtt_is_inited(void)
{
    return s_inited;
}

bool mymqtt_is_connected(void)
{
    return s_connected;
}

int mymqtt_publish(const char *topic, const void *data, size_t len, int qos)
{
    if (!s_inited || !s_connected) return -1;
    if (topic == NULL || data == NULL) return -1;

    return esp_mqtt_client_publish(s_hmqtt, topic, data, len, qos, 0);
}

esp_err_t mymqtt_subscribe(const char *topic, int qos)
{
    if (!s_inited || topic == NULL) return ESP_ERR_INVALID_ARG;

    int ret = esp_mqtt_client_subscribe(s_hmqtt, topic, qos);
    return (ret >= 0) ? ESP_OK : ESP_FAIL;
}

esp_err_t mymqtt_unsubscribe(const char *topic)
{
    if (!s_inited || topic == NULL) return ESP_ERR_INVALID_ARG;

    int ret = esp_mqtt_client_unsubscribe(s_hmqtt, topic);
    return (ret >= 0) ? ESP_OK : ESP_FAIL;
}

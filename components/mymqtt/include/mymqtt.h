#ifndef __MYMQTT_H__
#define __MYMQTT_H__

#include "mqtt_client.h"

/* MQTT配置 */
#define MQTT_URI       "mqtt://192.168.5.46:1883"
#define MQTT_CLIENT_ID "esp32s3_Client"
#define MQTT_RX_BUFFER_SIZE      40960      // 接收缓冲区大小
#define MQTT_RECONNECT_INTERVAL  10        // 重连间隔（秒）
#define DATA_TOPIC       "6050_date"       // 发送6050数据主题
#define IMG_TOPIC        "6818_image"      // 接收图像数据的主题
#define IMG_WIDTH        240
#define IMG_HEIGHT       240
#define IMG_PIXEL_SIZE   2
#define IMG_BUF_SIZE (IMG_WIDTH * IMG_HEIGHT * IMG_PIXEL_SIZE)

/**
 * @brief MQTT连接状态回调
 * @param connected 连接状态
 */
typedef void (*mqtt_conn_callback_t)(bool connected);

/**
 * @brief 初始化MQTT客户端
 * @param broker_uri 代理服务器地址
 * @param client_id  客户端标识符
 * @param rx_callback 数据接收回调（NULL表示使用默认处理）
 */
void mqtt_init(mqtt_conn_callback_t conn_cb);

/**
 * @brief 发布数据到指定主题
 * @param topic 目标主题
 * @param data  payload数据
 * @param len   数据长度
 * @param qos   服务质量等级（0-2）
 * @return 消息ID（失败返回-1）
 */
int mqtt_publish(const char *topic, const void *data, size_t len, int qos);

/**
 * @brief 订阅指定主题
 * @param topic 订阅主题
 * @param qos   服务质量等级
 * @return 成功返回ESP_OK
 */
esp_err_t mqtt_subscribe(const char *topic, int qos);

/**
 * @brief 查询MQTT连接状态
 * @return true表示已连接，false表示未连接
 */
bool mqtt_is_connected(void);

#endif //__MYMQTT_H__
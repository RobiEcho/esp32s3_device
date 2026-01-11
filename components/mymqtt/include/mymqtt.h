#ifndef __MYMQTT_H__
#define __MYMQTT_H__

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @brief 图像帧接收完成回调
 * @param image_data RGB565 图像数据（240x240）
 */
typedef void (*mymqtt_image_cb_t)(const uint16_t *image_data);

/**
 * @brief 初始化 MQTT 客户端
 * @param image_cb 图像帧完成回调（可为 NULL）
 * @return ESP_OK 成功
 */
esp_err_t mymqtt_init(mymqtt_image_cb_t image_cb);

bool mymqtt_is_inited(void);
bool mymqtt_is_connected(void);
int mymqtt_publish(const char *topic, const void *data, size_t len, int qos);
esp_err_t mymqtt_subscribe(const char *topic, int qos);
esp_err_t mymqtt_unsubscribe(const char *topic);

#endif /* __MYMQTT_H__ */

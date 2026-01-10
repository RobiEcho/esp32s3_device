#ifndef __ST7789_TASK_H__
#define __ST7789_TASK_H__

#include <stdint.h>
#include <stddef.h>
#include "st7789_config.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

/* ================= Task Configuration ================= */
#define ST7789_TASK_STACK_SIZE         (4096)
#define ST7789_TASK_PRIORITY           (5)
#define ST7789_TASK_FPS                (20)
#define ST7789_TASK_PERIOD_MS          (1000 / ST7789_TASK_FPS)
#define ST7789_TASK_QUEUE_LEN          (3)

/**
 * @brief 帧消息结构
 */
typedef struct {
    const uint16_t *data;       // 图像数据指针
    size_t size_bytes;          // 数据大小（字节）
} st7789_frame_msg_t;

/**
 * @brief 初始化 ST7789 显示任务
 * 
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t st7789_task_init(void);

/**
 * @brief 发送帧数据到显示队列
 * 
 * @param data 图像数据指针
 * @param size_bytes 数据大小（字节）
 * @return ESP_OK 成功，ESP_ERR_TIMEOUT 队列满，其他值表示失败
 */
esp_err_t st7789_task_send_frame(const uint16_t *data, size_t size_bytes);

#endif /* __ST7789_TASK_H__ */

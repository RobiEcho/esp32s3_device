#ifndef __MPU6050_TASK_H__
#define __MPU6050_TASK_H__

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "mpu6050.h"
#include "esp_err.h"

/* ================= Task Configuration ================= */
#define MPU6050_QUEUE_LEN               (32)
#define MPU6050_TASK_STACK_SIZE         (4096)
#define MPU6050_PRODUCER_TASK_PRIORITY  (5)
#define MPU6050_CONSUMER_TASK_PRIORITY  (4)

/* ================= External Queue Handle ================= */
extern QueueHandle_t g_mpu6050_queue;

/**
 * @brief 初始化 MPU6050 任务（生产者-消费者模式）
 * 
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t mpu6050_task_init(void);

#endif /* __MPU6050_TASK_H__ */

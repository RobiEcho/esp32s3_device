#ifndef __MPU6050_TASK_H__
#define __MPU6050_TASK_H__

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "mpu6050.h"

#define MPU6050_QUEUE_LEN               (32)
#define MPU6050_TASK_STACK_SIZE         (4096)
#define MPU6050_PRODUCER_TASK_PRIORITY  (5)

esp_err_t mpu6050_task_init(void);

/**
 * @brief 获取 MPU6050 数据队列句柄
 * @return 队列句柄，如果未初始化则返回 NULL
 */
QueueHandle_t mpu6050_task_get_queue(void);

#endif /* __MPU6050_TASK_H__ */

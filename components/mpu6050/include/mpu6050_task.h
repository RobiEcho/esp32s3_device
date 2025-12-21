#ifndef __MPU6050_TASK_H__
#define __MPU6050_TASK_H__

#include "mpu6050.h"

#define MPU6050_QUEUE_LEN               (32)
#define MPU6050_TASK_STACK_SIZE         (4096)
#define MPU6050_PRODUCER_TASK_PRIORITY  (5)

void mpu6050_task_init(void);

#endif // __MPU6050_TASK_H__

#include "mpu6050_task.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdio.h>

static const char *TAG = "MPU6050_Task";

static QueueHandle_t s_mpu6050_queue = NULL;

portTASK_FUNCTION(_mpu6050_producer_task, arg)
{
    mpu6050_data_t data;

    while (1) {
        if (mpu6050_read_data(&data) != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (xQueueSend(s_mpu6050_queue, &data, 0) != pdPASS) {
            ESP_LOGW(TAG, "MPU6050 queue full, drop data");
        }

        vTaskDelay(pdMS_TO_TICKS(10));  // 100Hz
    }
}

portTASK_FUNCTION(_mpu6050_consumer_task, arg)
{
    mpu6050_data_t data;

    while (1) {
        if (xQueueReceive(s_mpu6050_queue, &data, portMAX_DELAY) == pdTRUE) {

            // 转换为物理单位
            float gyro_x_dps = data.gyro_x / 131.0f;
            float gyro_y_dps = data.gyro_y / 131.0f;
            float gyro_z_dps = data.gyro_z / 131.0f;

            float accel_x_g = data.accel_x / 16384.0f;
            float accel_y_g = data.accel_y / 16384.0f;
            float accel_z_g = data.accel_z / 16384.0f;

            float accel_x_ms2 = accel_x_g * 9.80665f;
            float accel_y_ms2 = accel_y_g * 9.80665f;
            float accel_z_ms2 = accel_z_g * 9.80665f;

            // 打印
            ESP_LOGI(TAG,
                "Gyro[dps]  X: %.2f  Y: %.2f  Z: %.2f | "
                "Accel[m/s2] X: %.2f  Y: %.2f  Z: %.2f",
                gyro_x_dps, gyro_y_dps, gyro_z_dps,
                accel_x_ms2, accel_y_ms2, accel_z_ms2
            );
        }
    }
}

/* ===================== 初始化 ===================== */
void mpu6050_task_init(void)
{
    if (mpu6050_init() != ESP_OK) {
        ESP_LOGE(TAG, "MPU6050 init failed");
        return;
    }

    // 创建队列
    s_mpu6050_queue = xQueueCreate(MPU6050_QUEUE_LEN, sizeof(mpu6050_data_t));
    if (s_mpu6050_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create MPU6050 queue");
        return;
    }

    xTaskCreate(
        _mpu6050_producer_task,
        "mpu6050_producer",
        MPU6050_TASK_STACK_SIZE,
        NULL,
        MPU6050_PRODUCER_TASK_PRIORITY,
        NULL
    );

    xTaskCreate(
        _mpu6050_consumer_task,
        "mpu6050_consumer",
        MPU6050_TASK_STACK_SIZE,
        NULL,
        MPU6050_PRODUCER_TASK_PRIORITY,
        NULL
    );

    ESP_LOGI(TAG, "Tasks started");
}

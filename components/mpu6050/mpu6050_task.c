#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#include "mpu6050_task.h"

static const char *TAG = "mpu6050_task";

static QueueHandle_t s_mpu6050_queue = NULL;

static portTASK_FUNCTION(_mpu6050_producer_task, arg)
{
    mpu6050_data_t data;

    while (1) {
        if (mpu6050_read_data(&data) != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (xQueueSend(s_mpu6050_queue, &data, 0) != pdPASS) {
            ESP_LOGW(TAG, "mpu6050 data queue full, drop data");
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static portTASK_FUNCTION(_mpu6050_consumer_task, arg)
{
    mpu6050_data_t data;

    while (1) {
        if (xQueueReceive(s_mpu6050_queue, &data, portMAX_DELAY) == pdTRUE) {

            float gyro_x_dps = data.gyro_x / MPU6050_GYRO_LSB_PER_DPS;
            float gyro_y_dps = data.gyro_y / MPU6050_GYRO_LSB_PER_DPS;
            float gyro_z_dps = data.gyro_z / MPU6050_GYRO_LSB_PER_DPS;

            float accel_x_g = data.accel_x / MPU6050_ACCEL_LSB_PER_G;
            float accel_y_g = data.accel_y / MPU6050_ACCEL_LSB_PER_G;
            float accel_z_g = data.accel_z / MPU6050_ACCEL_LSB_PER_G;

            float accel_x_ms2 = accel_x_g * MPU6050_GRAVITY_MS2;
            float accel_y_ms2 = accel_y_g * MPU6050_GRAVITY_MS2;
            float accel_z_ms2 = accel_z_g * MPU6050_GRAVITY_MS2;

            ESP_LOGI(TAG,
                "Gyro[dps]  X: %.2f  Y: %.2f  Z: %.2f | "
                "Accel[m/s2] X: %.2f  Y: %.2f  Z: %.2f",
                gyro_x_dps, gyro_y_dps, gyro_z_dps,
                accel_x_ms2, accel_y_ms2, accel_z_ms2
            );
        }
    }
}

esp_err_t mpu6050_task_init(void)
{
    if (mpu6050_init() != ESP_OK) {
        ESP_LOGE(TAG, "mpu6050 init failed");
        return ESP_ERR_INVALID_STATE;
    }

    s_mpu6050_queue = xQueueCreate(MPU6050_QUEUE_LEN, sizeof(mpu6050_data_t));
    if (s_mpu6050_queue == NULL) {
        ESP_LOGE(TAG, "failed to create mpu6050 data_queue");
        return ESP_ERR_NO_MEM;
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

    ESP_LOGI(TAG, "tasks started");
    return ESP_OK;
}

QueueHandle_t mpu6050_task_get_queue(void)
{
    return s_mpu6050_queue;
}
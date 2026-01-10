#include "mpu6050_task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "mpu6050_task";
static bool s_task_inited = false;
QueueHandle_t g_mpu6050_queue = NULL;

static void _mpu6050_producer_task(void *arg)
{
    (void)arg;

    mpu6050_raw_data_t raw_data;
    const uint64_t read_interval = 1000000;  // 1000ms
    uint64_t last_read_time = esp_timer_get_time();
    
    while (1) {
        uint64_t current_time = esp_timer_get_time();
        if (current_time - last_read_time >= read_interval) {
            esp_err_t err = mpu6050_read_raw_data(&raw_data);
            if (err != ESP_OK) {
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }
            last_read_time = current_time;

            xQueueSend(g_mpu6050_queue, &raw_data, portMAX_DELAY);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void _mpu6050_consumer_task(void *arg)
{
    (void)arg;

    mpu6050_raw_data_t raw_data;
    mpu6050_data_t converted_data;

    while (1) {
        if (xQueueReceive(g_mpu6050_queue, &raw_data, portMAX_DELAY) == pdTRUE) {
            esp_err_t err = mpu6050_convert_data(&raw_data, &converted_data);
            if (err != ESP_OK) {
                continue;
            }

            ESP_LOGI(TAG, "陀螺仪[dps]  X: %.2f  Y: %.2f  Z: %.2f",
                     converted_data.gyro_x, converted_data.gyro_y, converted_data.gyro_z);
            ESP_LOGI(TAG, "加速度[m/s²] X: %.2f  Y: %.2f  Z: %.2f",
                     converted_data.accel_x, converted_data.accel_y, converted_data.accel_z);
        }
    }
}

esp_err_t mpu6050_task_init(void)
{
    // 初始化 MPU6050 驱动
    esp_err_t err = mpu6050_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MPU6050 初始化失败");
        return err;
    }

    // 校准陀螺仪（静止状态下进行）
    ESP_LOGI(TAG, "请保持设备静止，正在校准...");
    err = mpu6050_calibrate_gyro(NULL, 100);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "陀螺仪校准失败");
        return err;
    }

    // 创建数据队列
    g_mpu6050_queue = xQueueCreate(MPU6050_QUEUE_LEN, sizeof(mpu6050_raw_data_t));
    if (g_mpu6050_queue == NULL) {
        ESP_LOGE(TAG, "创建队列失败");
        return ESP_ERR_NO_MEM;
    }

    // 创建生产者任务
    BaseType_t ret = xTaskCreate(
        _mpu6050_producer_task,
        "mpu6050_producer",
        MPU6050_TASK_STACK_SIZE,
        NULL,
        MPU6050_PRODUCER_TASK_PRIORITY,
        NULL
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "创建生产者任务失败");
        return ESP_FAIL;
    }

    // 创建消费者任务
    ret = xTaskCreate(
        _mpu6050_consumer_task,
        "mpu6050_consumer",
        MPU6050_TASK_STACK_SIZE,
        NULL,
        MPU6050_CONSUMER_TASK_PRIORITY,
        NULL
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "创建消费者任务失败");
        return ESP_FAIL;
    }

    s_task_inited = true;
    ESP_LOGI(TAG, "任务初始化完成");
    return ESP_OK;
}
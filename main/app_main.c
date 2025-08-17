#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include <inttypes.h>
// #include "sdkconfig.h"
// #include "esp_chip_info.h"
// #include "esp_flash.h"
// #include "esp_system.h"
#include "esp_log.h"
#include "wifi_config.h"
#include "mpu6050.h"
#include "esp_timer.h"
#include "st7789.h"
#include "mymqtt.h"

static const char *TAG = "APP_Main";
static mpu6050_data_t gyro_bias = {0, 0, 0};

void mqtt_send_data(float angles[]) {
    char json_buffer[100];
    int len = snprintf(json_buffer, sizeof(json_buffer), 
                      "{\"angle_y\": %.2f, \"angle_z\": %.2f}",
                      angles[0], angles[1]);
    if (len <= 0 || len >= sizeof(json_buffer)) {
        ESP_LOGE(TAG, "JSON字符串生成失败或缓冲区不足");
        return;
    }
    mqtt_publish(IMG_TOPIC, json_buffer, len, 1);
}

void app_main(void)
{
    ESP_LOGI(TAG, "开始初始化");
    wifi_init(NULL);
    vTaskDelay(pdMS_TO_TICKS(5000));
    mqtt_init(NULL);

    st7789_init();
    st7789_fill_screen(0xFFFF);// 清屏为白色

    mpu6050_init();
    mpu6050_calibrate_gyro(&gyro_bias, 100);  // 校准陀螺仪偏移
    float angles[2] = {90.0f,90.0f};  // Y/Z轴角度（对应舵机初始位置）
    int64_t last_time = esp_timer_get_time();

    while (1)
    {
        // 读取传感器数据
        mpu6050_data_t raw;
        mpu6050_read_gyro(&raw);
        
        // 计算角度变化
        float dt = (esp_timer_get_time() - last_time) / 1e6;
        angles[0] = mpu6050_calculate_angle(angles[0], (raw.y - gyro_bias.y) , dt);
        angles[1] = mpu6050_calculate_angle(angles[1], (raw.z - gyro_bias.z) , dt);
        last_time = esp_timer_get_time();

        // printf("angle_y: %.2f, angle_z: %.2f\n", angles[0], angles[1]);
        // 发布数据到MQTT
        mqtt_send_data(angles);
        vTaskDelay(pdMS_TO_TICKS(100));// 延时100ms（0.1s）
    }
}
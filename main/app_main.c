#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include <inttypes.h>
// #include "sdkconfig.h"
// #include "esp_chip_info.h"
// #include "esp_flash.h"
// #include "esp_system.h"
#include "esp_log.h"
// #include "wifi_config.h"
// #include "mpu6050.h"
#include "esp_timer.h"
#include "st7789.h"
#include "mymqtt.h"

static const char *TAG = "APP_Main";
// static mpu6050_data_t gyro_bias = {0, 0, 0};

void app_main(void)
{
    ESP_LOGI(TAG, "开始初始化");

    st7789_init();
    st7789_fill_screen(0xFFFF);

    vTaskDelay(pdMS_TO_TICKS(5000));

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    /*
    wifi_init(NULL);
    vTaskDelay(pdMS_TO_TICKS(5000));// WiFi初始化，成功连接网络后对设备进行初始化
    
    mpu6050_init();

    mpu6050_calibrate_gyro(&gyro_bias, 100);  // 校准陀螺仪偏移
    float angles[2] = {90.0f,90.0f};  // Y/Z轴角度
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

        printf("angle_y: %.2f, angle_z: %.2f\n", angles[0], angles[1]);
        vTaskDelay(pdMS_TO_TICKS(100));// 延时100ms（0.1s）
    }
    */
}

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "config.h"
#include "mpu6050.h"
#include "wifi_mod.h"
#include "st7789.h"
#include "mqtt.h"

static const char *TAG = "APP_Main";
static mpu6050_data_t gyro_bias = {0, 0, 0};

// wifi连接成功后, 初始化MQTT
static void wifi_connected_callback(void) {
    mqtt_comm_init(NULL);
}

// // MQTT图像接收处理
// static void display_image_callback(const uint8_t *data, size_t len) {
//     st7789_display_raw(data, len);
//     ESP_LOGI(TAG, "已刷新显示屏图像");
// }

void app_main(void) {
    ESP_LOGI(TAG, "开始初始化");
    wifi_init(wifi_connected_callback);
    vTaskDelay(pdMS_TO_TICKS(5000));// WiFi初始化，成功连接网络后对设备进行初始化
    mpu6050_init();
    st7789_init();
    st7789_fill_screen(0xFFFF);  // 初始化时填充白色
    
    mpu6050_calibrate_gyro(&gyro_bias, 100);  // 校准陀螺仪偏移
    float angles[2] = {90.0f,90.0f};  // Y/Z轴角度
    int64_t last_time = esp_timer_get_time();

    while (1) {
        // 读取传感器数据
        mpu6050_data_t raw;
        mpu6050_read_gyro(&raw);
        
        // 计算角度变化
        float dt = (esp_timer_get_time() - last_time) / 1e6;
        angles[0] = mpu6050_calculate_angle(angles[0], (raw.y - gyro_bias.y) , dt);
        angles[1] = mpu6050_calculate_angle(angles[1], (raw.z - gyro_bias.z) , dt);
        last_time = esp_timer_get_time();

        printf("angle_y: %.2f, angle_z: %.2f\n", angles[0], angles[1]);

        // 发送MQTT数据
        char payload[64];
        snprintf(payload, sizeof(payload), 
                "{\"angle_y\":%.2f,\"angle_z\":%.2f}", 
                angles[0], angles[1]);
        mqtt_publish(MQTT_TOPIC, payload, strlen(payload), 1);

        vTaskDelay(pdMS_TO_TICKS(100));// 延时100ms（0.1s）
    }
}
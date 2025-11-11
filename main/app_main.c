#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "wifi_config.h"
#include "mpu6050.h"
#include "esp_timer.h"
#include "st7789.h"
#include "mymqtt.h"

static const char *TAG = "APP_Main";
static mpu6050_data_t gyro_bias = {0, 0, 0};

// 事件组标志位
static EventGroupHandle_t init_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define MQTT_CONNECTED_BIT BIT1

// 传感器数据采集任务句柄
static TaskHandle_t sensor_task_handle = NULL;

// WiFi 连接回调
static void wifi_connected_callback(void) {
    ESP_LOGI(TAG, "WiFi 连接成功，准备初始化 MQTT");
    xEventGroupSetBits(init_event_group, WIFI_CONNECTED_BIT);
}

// MQTT 连接回调
static void mqtt_connected_callback(bool connected) {
    if (connected) {
        ESP_LOGI(TAG, "MQTT 连接成功，开始传感器数据采集");
        xEventGroupSetBits(init_event_group, MQTT_CONNECTED_BIT);
    } else {
        ESP_LOGW(TAG, "MQTT 断开连接");
        xEventGroupClearBits(init_event_group, MQTT_CONNECTED_BIT);
    }
}

// 发送传感器数据
static void mqtt_send_data(float angles[]) {
    // 只在 MQTT 连接时发送
    if (!mqtt_is_connected()) {
        return;
    }
    
    char json_buffer[100];
    int len = snprintf(json_buffer, sizeof(json_buffer), 
                      "{\"angle_y\": %.1f, \"angle_z\": %.1f}",
                      angles[0], angles[1]);
    if (len <= 0 || len >= sizeof(json_buffer)) {
        ESP_LOGE(TAG, "JSON字符串生成失败或缓冲区不足");
        return;
    }
    mqtt_publish(DATA_TOPIC, json_buffer, len, 1);
}

// 传感器数据采集任务
static void sensor_task(void *arg) {
    float angles[2] = {90.0f, 90.0f};
    int64_t last_time = esp_timer_get_time();
    
    ESP_LOGI(TAG, "传感器任务启动");
    
    while (1) {
        // 读取传感器数据
        mpu6050_data_t raw;
        mpu6050_read_gyro(&raw);
        
        // 计算角度
        float dt = (esp_timer_get_time() - last_time) / 1e6;
        angles[0] = mpu6050_calculate_angle(angles[0], (raw.y - gyro_bias.y), dt);
        angles[1] = mpu6050_calculate_angle(angles[1], (raw.z - gyro_bias.z), dt);
        last_time = esp_timer_get_time();
        // printf("angle_y: %.2f, angle_z: %.2f\n", angles[0], angles[1]);
        // 发布数据到MQTT
        mqtt_send_data(angles);
        
        vTaskDelay(pdMS_TO_TICKS(50));  // 延时50ms
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "开始初始化");
    
    // 创建事件组
    init_event_group = xEventGroupCreate();
    
    // 初始化显示屏
    st7789_init();
    st7789_fill_screen(0xFFFF);  // 清屏为白色
    
    // 初始化 WiFi（使用回调通知连接成功）
    wifi_init_sta(wifi_connected_callback);
    
    // 等待 WiFi 连接成功
    EventBits_t bits = xEventGroupWaitBits(
        init_event_group,       // 事件组句柄
        WIFI_CONNECTED_BIT,     // 等待哪些位
        pdFALSE,                // 等待成功后不自动清除
        pdTRUE,                 // 等待所有指定位都为1
        pdMS_TO_TICKS(10000)    // 超时10秒
    );
    
    if (!(bits & WIFI_CONNECTED_BIT)) {
        ESP_LOGW(TAG, "WiFi 连接超时");
        // 阻塞研究情况
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
    // 初始化 MQTT
    mqtt_init(mqtt_connected_callback);
    
    // 初始化传感器（在创建任务前初始化）
    mpu6050_init();
    mpu6050_calibrate_gyro(&gyro_bias, 100);  // 校准陀螺仪偏移
    
    // 创建传感器数据采集任务
    xTaskCreate(
        sensor_task,
        "sensor_task",
        4096,
        NULL,
        5,
        &sensor_task_handle
    );
    
    ESP_LOGI(TAG, "初始化完成，系统运行中...");
}
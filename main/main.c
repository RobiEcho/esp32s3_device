#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi.h"
#include "mymqtt.h"
#include "st7789.h"

static const char *TAG = "main";

// 图像帧接收完成回调
static void _image_cb(const uint16_t *image_data)
{
    ESP_LOGI(TAG, "绘制图像");
    st7789_draw_image(image_data);
}

void app_main(void)
{
    ESP_LOGI(TAG, "应用启动");
    
    // 初始化 ST7789
    ESP_ERROR_CHECK(st7789_init());
    st7789_fill_screen(0xFFFF);  // 清屏



    // 启动 WiFi
    ESP_ERROR_CHECK(wifi_start());

    // 等待 WiFi 连接
    while (wifi_get_state() != WIFI_STATE_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    vTaskDelay(pdMS_TO_TICKS(3000));
    // 初始化 MQTT（传入图像回调）
    ESP_ERROR_CHECK(mymqtt_init(_image_cb));

    ESP_LOGI(TAG, "等待图像数据...");

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

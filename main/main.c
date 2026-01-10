#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "st7789.h"
#include "firefly.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "应用启动");
    
    // 初始化 ST7789
    esp_err_t err = st7789_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ST7789 初始化失败");
        return;
    }

    // 清屏
    st7789_fill_screen(0x0000);
    vTaskDelay(pdMS_TO_TICKS(500));

    // 测试 st7789_draw_image（使用 Ping-Pong 缓冲区）
    ESP_LOGI(TAG, "显示 firefly 图像");
    st7789_draw_image(firefly_rgb);

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

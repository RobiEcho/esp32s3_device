#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "lvgl_task.h"

static const char *TAG = "Main";

void app_main(void)
{
    ESP_LOGI(TAG, "启动 LVGL 任务...");

    /* 初始化并启动LVGL任务 */
    esp_err_t ret = lvgl_task_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init LVGL task");
        return;
    }

    ESP_LOGI(TAG, "LVGL task started successfully");
}
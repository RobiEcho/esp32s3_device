#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "lvgl_task.h"

static const char *TAG = "main";

void app_main(void)
{

    esp_err_t ret = lvgl_task_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to init LVGL task");
        return;
    }

    ESP_LOGI(TAG, "lvgl task started successfully");
}
#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "lvgl_port.h"

static const char *TAG = "Main";

void app_main(void)
{
    ESP_LOGI(TAG, "启动 LVGL Demo...");

    /* 直接进入 LVGL Demo（内部会初始化 LVGL、显示驱动并进入循环） */
    lvgl_demo();
}
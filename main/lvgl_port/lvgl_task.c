#include "lvgl_task.h"
#include "lvgl_ui_config.h"
#include "lv_port_disp.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "LVGL_TASK";

static TaskHandle_t s_lvgl_task_handle = NULL;

/**
 * @brief LVGL任务主函数
 */
static void lvgl_task_func(void *arg)
{
    (void)arg;

    ESP_LOGI(TAG, "LVGL task started");

    /* 初始化LVGL */
    lv_init();

    /* 初始化显示驱动 */
    lv_port_disp_init();

    /* 配置LVGL时钟 */
    if (lvgl_ui_config_tick() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to config LVGL tick");
        vTaskDelete(NULL);
        return;
    }

    /* 创建UI界面 */
    lvgl_ui_create();

    /* 任务主循环 */
    TickType_t last_wake = xTaskGetTickCount();
    while (1) {
        lv_timer_handler();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(LVGL_TASK_PERIOD_MS));
    }
}

/**
 * @brief 初始化并启动LVGL任务
 */
esp_err_t lvgl_task_init(void)
{
    if (s_lvgl_task_handle != NULL) {
        ESP_LOGW(TAG, "LVGL task already initialized");
        return ESP_OK;
    }

    BaseType_t ret = xTaskCreate(
        lvgl_task_func,
        "lvgl_task",
        LVGL_TASK_STACK_SIZE,
        NULL,
        LVGL_TASK_PRIORITY,
        &s_lvgl_task_handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LVGL task");
        s_lvgl_task_handle = NULL;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "LVGL task initialized");
    return ESP_OK;
}

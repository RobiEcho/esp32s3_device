#include "st7789_task.h"
#include "st7789.h"
#include "esp_log.h"
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "st7789_task";

static QueueHandle_t s_frame_queue = NULL;
static TaskHandle_t s_task_handle = NULL;
static bool s_task_inited = false;

static bool _st7789_frame_valid(const st7789_frame_msg_t *frame)
{
    if (frame == NULL) {
        return false;
    }
    if (frame->data == NULL) {
        return false;
    }
    return true;
}

static portTASK_FUNCTION(_st7789_task, arg)
{
    (void)arg;

    TickType_t last_wake = xTaskGetTickCount();
    st7789_frame_msg_t frame = { 0 };

    while (1) {
        if (s_frame_queue == NULL) {
            vTaskDelay(pdMS_TO_TICKS(ST7789_TASK_PERIOD_MS));
            continue;
        }

        if (xQueueReceive(s_frame_queue, &frame, portMAX_DELAY) == pdTRUE) {
            if (_st7789_frame_valid(&frame)) {
                st7789_draw_image(frame.data);
            }
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(ST7789_TASK_PERIOD_MS));
    }
}

esp_err_t st7789_task_init(void)
{
    if (s_task_inited) {
        return ESP_OK;
    }

    // 初始化 ST7789 驱动
    esp_err_t err = st7789_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ST7789 初始化失败");
        return err;
    }

    // 创建帧队列
    s_frame_queue = xQueueCreate(ST7789_TASK_QUEUE_LEN, sizeof(st7789_frame_msg_t));
    if (s_frame_queue == NULL) {
        ESP_LOGE(TAG, "创建帧队列失败");
        return ESP_ERR_NO_MEM;
    }

    // 创建显示任务
    BaseType_t ret = xTaskCreate(
        _st7789_task,
        "st7789_task",
        ST7789_TASK_STACK_SIZE,
        NULL,
        ST7789_TASK_PRIORITY,
        &s_task_handle
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "创建任务失败");
        return ESP_FAIL;
    }

    s_task_inited = true;
    ESP_LOGI(TAG, "任务初始化完成");
    return ESP_OK;
}

esp_err_t st7789_task_send_frame(const uint16_t *data, size_t size_bytes)
{
    if (!s_task_inited || s_frame_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    st7789_frame_msg_t frame = {
        .data = data,
        .size_bytes = size_bytes
    };

    if (xQueueSend(s_frame_queue, &frame, 0) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

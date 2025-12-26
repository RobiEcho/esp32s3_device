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
    if (s_task_handle != NULL) {
        return ESP_OK;
    }

    if (st7789_init() != ESP_OK) {
        ESP_LOGE(TAG, "st7789 init failed");
        return ESP_ERR_INVALID_STATE;
    }

    s_frame_queue = xQueueCreate(ST7789_TASK_QUEUE_LEN, sizeof(st7789_frame_msg_t));
    if (s_frame_queue == NULL) {
        ESP_LOGE(TAG, "failed to create frame queue");
        return ESP_ERR_NO_MEM;
    }

    xTaskCreate(
        _st7789_task,
        "st7789_task",
        ST7789_TASK_STACK_SIZE,
        NULL,
        ST7789_TASK_PRIORITY,
        &s_task_handle
    );

    ESP_LOGI(TAG, "task started");
    return ESP_OK;
}

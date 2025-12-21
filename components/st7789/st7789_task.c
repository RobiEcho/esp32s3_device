#include "st7789_task.h"
#include "st7789.h"
#include "esp_log.h"
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "ST7789_Task";

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

portTASK_FUNCTION(_st7789_task, arg)
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

esp_err_t st7789_task_send_frame(const st7789_frame_msg_t *frame, TickType_t ticks_to_wait)
{
    if (!_st7789_frame_valid(frame)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_frame_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xQueueSend(s_frame_queue, frame, ticks_to_wait) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

esp_err_t st7789_task_init(void)
{
    if (s_task_handle != NULL) {
        return ESP_OK;
    }

    if (!st7789_is_inited()) {
        esp_err_t err = st7789_init();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "ST7789 init failed");
            return err;
        }
    }

    if (s_frame_queue == NULL) {
        s_frame_queue = xQueueCreate(ST7789_TASK_QUEUE_LEN, sizeof(st7789_frame_msg_t));
        if (s_frame_queue == NULL) {
            ESP_LOGE(TAG, "Failed to create frame queue");
            return ESP_ERR_NO_MEM;
        }
    }

    if (xTaskCreate(
            _st7789_task,
            "st7789_task",
            ST7789_TASK_STACK_SIZE,
            NULL,
            ST7789_TASK_PRIORITY,
            &s_task_handle
        ) != pdPASS) {
        s_task_handle = NULL;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Task started");
    return ESP_OK;
}

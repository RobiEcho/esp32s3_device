#ifndef __ST7789_TASK_H__
#define __ST7789_TASK_H__

#include <stdint.h>
#include <stddef.h>
#include "st7789_config.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#define ST7789_TASK_STACK_SIZE         (4096)
#define ST7789_TASK_PRIORITY           (5)
#define ST7789_TASK_FPS                (20)
#define ST7789_TASK_PERIOD_MS          (1000 / ST7789_TASK_FPS)
#define ST7789_TASK_QUEUE_LEN          (3)
#define ST7789_FRAME_WIDTH             (ST7789_WIDTH)
#define ST7789_FRAME_HEIGHT            (ST7789_HEIGHT)
#define ST7789_FRAME_BPP               (2)
#define ST7789_FRAME_SIZE_BYTES        (ST7789_FRAME_WIDTH * ST7789_FRAME_HEIGHT * ST7789_FRAME_BPP)

typedef struct {
    const uint16_t *data;
    size_t size_bytes;
} st7789_frame_msg_t;

esp_err_t st7789_task_init(void);

esp_err_t st7789_task_send_frame(const st7789_frame_msg_t *frame, TickType_t ticks_to_wait);

#endif // __ST7789_TASK_H__

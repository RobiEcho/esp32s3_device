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

typedef struct {
    const uint16_t *data;
    size_t size_bytes;
} st7789_frame_msg_t;

esp_err_t st7789_task_init(void);

#endif /* __ST7789_TASK_H__ */

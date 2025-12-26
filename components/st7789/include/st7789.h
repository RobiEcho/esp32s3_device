#ifndef __ST7789_DRIVER_H__
#define __ST7789_DRIVER_H__

#include "st7789_config.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#if ST7789_PINGPONG_BUFFER_ENABLE
typedef enum {
    PINGPONG_BUF_IDLE = 0,
    PINGPONG_BUF_FILLING,
    PINGPONG_BUF_READY,
    PINGPONG_BUF_SENDING
} st7789_pingpong_buf_status_t;

typedef struct {
    uint16_t *buf[ST7789_PINGPONG_BUF_COUNT];   // [0]=PING, [1]=PONG
    size_t    buf_size;                    // 单个缓冲区可用像素数
    uint8_t   cpu_idx;                     // CPU 正在写的 buffer index
    volatile st7789_pingpong_buf_status_t status[ST7789_PINGPONG_BUF_COUNT];
} st7789_pingpong_t;
#endif

esp_err_t st7789_init(void);
bool st7789_is_inited(void);

/**
 * @brief 清屏操作
 * @param color 16位RGB颜色值
 */
void st7789_fill_screen(uint16_t color);

/**
 * @brief 绘制RGB565图像
 * @param image_data 图像数据指针
 */
void st7789_draw_image(const uint16_t *image_data);

/**
 * @brief 用于 LVGL 刷新图像
 */
void st7789_draw_area(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t *color_map);

#endif //__ST7789_DRIVER_H__
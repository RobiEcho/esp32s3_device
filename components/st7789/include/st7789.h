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
    size_t    buf_size;                         // 单个缓冲区可用像素数
    volatile st7789_pingpong_buf_status_t status[ST7789_PINGPONG_BUF_COUNT];
} st7789_pingpong_t;
#endif

esp_err_t st7789_init(void);
bool st7789_is_inited(void);
void st7789_sleep(void);
void st7789_wakeup(void);
void st7789_display_on(void);
void st7789_display_off(void);

/**
 * @brief 清屏（填充单色）
 * 
 * @param color RGB565 颜色值
 */
void st7789_fill_screen(uint16_t color);

/**
 * @brief 绘制全屏 RGB565 图像
 * 
 * @param image_data 图像数据指针（240x240 像素）
 */
void st7789_draw_image(const uint16_t *image_data);

/**
 * @brief 在指定区域绘制 RGB565 图像（适配 LVGL）
 * 
 * @param x1 起始列
 * @param y1 起始行
 * @param x2 结束列
 * @param y2 结束行
 * @param color_map 颜色数据指针
 */
void st7789_draw_area(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t *color_map);

#endif //__ST7789_DRIVER_H__
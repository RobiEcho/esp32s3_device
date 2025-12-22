#ifndef __ST7789_DRIVER_H__
#define __ST7789_DRIVER_H__

#include "st7789_config.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define ST7789_DMA_BUF_COUNT  2

typedef enum {
    DMA_BUF_IDLE = 0,   // 空闲：CPU/DMA 都没在用
    DMA_BUF_FILLING,    // CPU 正在往里填数据
    DMA_BUF_READY,      // CPU 填完，DMA 可以用
    DMA_BUF_SENDING     // DMA 正在发送
} st7789_dma_buf_status_t;

typedef struct {
    uint16_t *buf[ST7789_DMA_BUF_COUNT];   // [0]=PING, [1]=PONG
    size_t    buf_size;                    // 单个缓冲区可用像素数
    uint8_t   cpu_idx;                     // CPU 正在写的 buffer index
    volatile st7789_dma_buf_status_t status[ST7789_DMA_BUF_COUNT];
} st7789_dma_pingpong_t;

/**
 * @brief  初始化ST7789显示屏
 * @note  包含GPIO、SPI总线和设备初始化
 */
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
 * @brief 在指定区域绘制 RGB565 图像（用于 LVGL 刷新）
 * @param x1, y1 左上角坐标（包含）
 * @param x2, y2 右下角坐标（包含）
 * @param color_map 像素数据指针，大小为 (x2-x1+1)*(y2-y1+1)
 * @note 接受 int32_t 类型坐标，兼容 LVGL 的 lv_coord_t
 */
void st7789_draw_area(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t *color_map);

#endif //__ST7789_DRIVER_H__
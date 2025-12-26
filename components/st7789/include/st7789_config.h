#ifndef ST7789_DRIVER_CONFIG_H
#define ST7789_DRIVER_CONFIG_H

/* ================= ST7789 Hardware Config ================= */

#define ST7789_SPI_HOST              SPI3_HOST    // SPI3
#define ST7789_SPI_MOSI_PIN          47           // SPI数据线
#define ST7789_SPI_SCLK_PIN          21           // SPI时钟线

#define ST7789_RES_PIN               45            // 复位引脚
#define ST7789_DC_PIN                40            // 数据/命令控制引脚

#define ST7789_SPI_MODE              3            // ST7789 requires SPI mode 0
#define ST7789_SPI_CLOCK_HZ          (20 * 1000 * 1000)  // SPI时钟频率
#define ST7789_SPI_QUEUE_SIZE        7            // SPI事务队列

#define ST7789_WIDTH                 240          // 分辨率
#define ST7789_HEIGHT                240

/* ============== ST7789 Pixel Format ============== */
#define ST7789_PIXEL_BPP             2            // RGB565: 2字节/像素
#define ST7789_FRAME_SIZE_BYTES      (ST7789_WIDTH * ST7789_HEIGHT * ST7789_PIXEL_BPP)  // 一帧完整大小（字节）
#define ST7789_SPI_MAX_TRANS_SIZE    (ST7789_FRAME_SIZE_BYTES / 10)  // 一帧的 1/10，用于分块传输

/* ============== ST7789 DMA configuration ============== */
/* 每次DMA传输的最大像素数（RGB565 = 2字节/像素） */
#define ST7789_DMA_MAX_PIXELS      (ST7789_SPI_MAX_TRANS_SIZE / 2)
#define ST7789_PINGPONG_BUFFER_ENABLE  0

/* ================= ST7789 Command Set ================= */

#define ST7789_CMD_SLEEP_OUT         0x11       // 退出睡眠模式
#define ST7789_CMD_INVON             0x21       // 开启颜色反转
#define ST7789_CMD_INVOFF            0x20       // 关闭颜色反转
#define ST7789_CMD_COLMOD            0x3A       // 设置颜色模式
#define ST7789_CMD_MADCTL            0x36       // 设置内存访问方向
#define ST7789_CMD_DISPLAY_ON        0x29       // 开启显示
#define ST7789_PIXEL_FORMAT          0x55       // 16-bit RGB565
#define ST7789_MADCTL_RGB_V          0x60       // RGB顺序，垂直翻转
#define ST7789_CMD_CASET             0x2A       // 列地址设置
#define ST7789_CMD_RASET             0x2B       // 行地址设置
#define ST7789_CMD_RAMWR             0x2C       // 内存写入 

#if ST7789_PINGPONG_BUFFER_ENABLE
#define ST7789_PINGPONG_BUF_COUNT  2

enum {
    PINGPONG_BUF_PING = 0,
    PINGPONG_BUF_PONG
};
#endif

#endif /* ST7789_DRIVER_CONFIG_H */

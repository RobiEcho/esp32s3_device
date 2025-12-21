#ifndef ST7789_DRIVER_CONFIG_H
#define ST7789_DRIVER_CONFIG_H

/* ================= ST7789 Hardware Config ================= */

#define ST7789_SPI_HOST              SPI3_HOST    // SPI3
#define ST7789_SPI_MOSI_PIN          11           // SPI数据线
#define ST7789_SPI_SCLK_PIN          12           // SPI时钟线

#define ST7789_RES_PIN               6            // 复位引脚
#define ST7789_DC_PIN                7            // 数据/命令控制引脚

#define ST7789_SPI_MODE              3            // ST7789 requires SPI mode 0
#define ST7789_SPI_CLOCK_HZ          (40 * 1000 * 1000)  // SPI时钟频率
#define ST7789_SPI_QUEUE_SIZE        7            // SPI事务队列
#define ST7789_SPI_MAX_TRANS_SIZE    4096         // SPI最大传输字节数

#define ST7789_WIDTH                 240          // 分辨率
#define ST7789_HEIGHT                240

/* ============== ST7789 DMA configuration ============== */
#define ST7789_MAX_TRANS_SIZE       4096         // 每次DMA传输的最大字节数

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

#define ST7789_DMA_BUF_COUNT  2

enum {
    DMA_BUF_PING = 0,
    DMA_BUF_PONG
};

#endif /* ST7789_DRIVER_CONFIG_H */

#ifndef ST7789_DRIVER_CONFIG_H
#define ST7789_DRIVER_CONFIG_H

/* ================= Hardware Pin Config ================= */
#define ST7789_SPI_HOST              SPI3_HOST           // SPI3
#define ST7789_SPI_MOSI_PIN          47                  // SPI 数据线
#define ST7789_SPI_SCLK_PIN          21                  // SPI 时钟线
#define ST7789_RES_PIN               45                  // 复位引脚
#define ST7789_DC_PIN                40                  // 数据/命令控制引脚

/* ================= SPI Config ================= */
#define ST7789_SPI_MODE              3                   // SPI 模式 3 (CPOL=1, CPHA=1)
#define ST7789_SPI_CLOCK_HZ          (20 * 1000 * 1000)  // SPI 时钟频率 20MHz
#define ST7789_SPI_QUEUE_SIZE        7                   // SPI 事务队列大小

/* ================= Display Config ================= */
#define ST7789_WIDTH                 240                 // 屏幕宽度
#define ST7789_HEIGHT                240                 // 屏幕高度

/* ================= Pixel Format ================= */
#define ST7789_PIXEL_BPP             2                   // RGB565: 2 字节/像素
#define ST7789_FRAME_BYTES           (ST7789_WIDTH * ST7789_HEIGHT * ST7789_PIXEL_BPP)  // 一帧大小 (字节)
#define ST7789_MAX_TRANS_BYTES       (ST7789_FRAME_BYTES / 10)                          // 单次传输大小 (字节)

/* ================= DMA Config ================= */
#define ST7789_PINGPONG_BUFFER_ENABLE  1               // Ping-Pong 双缓冲开关 (0=关闭, 1=开启)

#if ST7789_PINGPONG_BUFFER_ENABLE
#define ST7789_PINGPONG_BUF_COUNT    2                   // 双缓冲数量
#define ST7789_PINGPONG_BUF_BYTES    ST7789_MAX_TRANS_BYTES  // 单个缓冲区大小 (字节)
enum {
    PINGPONG_BUF_PING = 0,
    PINGPONG_BUF_PONG
};
#endif

/* ================= Command Set ================= */
#define ST7789_CMD_NOP               0x00       // 空操作
#define ST7789_CMD_SWRESET           0x01       // 软件复位
#define ST7789_CMD_SLEEP_IN          0x10       // 进入睡眠模式
#define ST7789_CMD_SLEEP_OUT         0x11       // 退出睡眠模式
#define ST7789_CMD_INVOFF            0x20       // 关闭颜色反转
#define ST7789_CMD_INVON             0x21       // 开启颜色反转
#define ST7789_CMD_DISPLAY_OFF       0x28       // 关闭显示
#define ST7789_CMD_DISPLAY_ON        0x29       // 开启显示
#define ST7789_CMD_CASET             0x2A       // 列地址设置
#define ST7789_CMD_RASET             0x2B       // 行地址设置
#define ST7789_CMD_RAMWR             0x2C       // 写入显存
#define ST7789_CMD_MADCTL            0x36       // 内存访问控制
#define ST7789_CMD_COLMOD            0x3A       // 颜色模式

/* ================= Command Parameters ================= */
#define ST7789_PIXEL_FORMAT          0x55       // 16-bit RGB565
#define ST7789_MADCTL_RGB_V          0xC0       // RGB,显示方向 (0x00=0°, 0x60=90°, 0xC0=180°, 0xA0=270°)

/* 显存偏移（240x240 屏幕使用 240x320 控制器，根据旋转方向自动设置） */
#if (ST7789_MADCTL_RGB_V == 0xC0)
    #define ST7789_X_OFFSET          0
    #define ST7789_Y_OFFSET          80
#elif (ST7789_MADCTL_RGB_V == 0xA0)
    #define ST7789_X_OFFSET          80
    #define ST7789_Y_OFFSET          0
#else
    #define ST7789_X_OFFSET          0
    #define ST7789_Y_OFFSET          0
#endif

#endif /* ST7789_DRIVER_CONFIG_H */

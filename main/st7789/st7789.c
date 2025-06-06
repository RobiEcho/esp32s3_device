#include "st7789.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"

static const char *TAG = "ST7789";
static spi_device_handle_t spi_handle = NULL; //  定义一个spi设备句柄，用于存储spi设备句柄

/* 内部函数声明 */
static void send_cmd(uint8_t cmd);
static void send_data(const uint8_t *data, size_t len);
static void hardware_reset(void);
static void set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

void st7789_init(void) {
    // GPIO初始化
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ST7789_DC_PIN) | (1ULL << ST7789_RES_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // SPI总线配置
    spi_bus_config_t buscfg = {
        .mosi_io_num = ST7789_MOSI_PIN,
        .miso_io_num = -1,
        .sclk_io_num = ST7789_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = ST7789_MAX_TRANS_SIZE * 2
    };

    // SPI设备配置
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40 * 1000 * 1000, // 40MHz时钟
        .mode = 3,                          // SPI模式3
        .spics_io_num = -1,                 // 不使用CS引脚
        .queue_size = 7,
        .flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_NO_DUMMY
    };

    // 初始化SPI总线及设备
    ESP_ERROR_CHECK(spi_bus_initialize(ST7789_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(ST7789_SPI_HOST, &devcfg, &spi_handle));

    // 硬件复位序列
    hardware_reset();

    // 初始化命令序列
    send_cmd(0x11); // 退出睡眠模式
    vTaskDelay(pdMS_TO_TICKS(120));

    send_cmd(0x3A); // 设置颜色模式
    uint8_t mode = ST7789_PIXEL_FORMAT;
    send_data(&mode, 1);

    send_cmd(0x36); // 设置内存访问方向
    uint8_t madctl = 0x60; // RGB顺序，垂直翻转
    send_data(&madctl, 1);

    send_cmd(0x29); // 开启显示
    ESP_LOGI(TAG, "显示屏初始化完成");
}

// 静态函数，用于硬件复位
static void hardware_reset(void) {
    // 将ST7789_RES_PIN引脚电平设置为0，即低电平
    gpio_set_level(ST7789_RES_PIN, 0);
    // 延时20毫秒
    vTaskDelay(pdMS_TO_TICKS(20));
    // 将ST7789_RES_PIN引脚电平设置为1，即高电平
    gpio_set_level(ST7789_RES_PIN, 1);
    // 延时120毫秒
    vTaskDelay(pdMS_TO_TICKS(120));
}

static void send_cmd(uint8_t cmd) {
    gpio_set_level(ST7789_DC_PIN, 0);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi_handle, &t));
}

static void send_data(const uint8_t *data, size_t len) {
    gpio_set_level(ST7789_DC_PIN, 1);
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi_handle, &t));
}

// 设置显示窗口
static void set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t buf[4];
    
    // 设置列地址
    send_cmd(0x2A);
    buf[0] = x0 >> 8; buf[1] = x0 & 0xFF;
    buf[2] = x1 >> 8; buf[3] = x1 & 0xFF;
    send_data(buf, 4);

    // 设置行地址
    send_cmd(0x2B);
    buf[0] = y0 >> 8; buf[1] = y0 & 0xFF;
    buf[2] = y1 >> 8; buf[3] = y1 & 0xFF;
    send_data(buf, 4);

    send_cmd(0x2C); // 内存写入命令
}

// 清屏
void st7789_fill_screen(uint16_t color) {
    uint8_t color_byte[2];
    color_byte[0] = ~(color >> 8);  // 根据硬件特性取反
    color_byte[1] = ~(color & 0xFF);

    set_address_window(0, 0, ST7789_WIDTH-1, ST7789_HEIGHT-1);

    // 准备单行缓冲区
    uint8_t line_buffer[ST7789_WIDTH * 2];
    for(int i=0; i<sizeof(line_buffer); i+=2) {
        line_buffer[i] = color_byte[0];
        line_buffer[i+1] = color_byte[1];
    }

    // 逐行填充
    for(int y=0; y<ST7789_HEIGHT; y++) {
        send_data(line_buffer, sizeof(line_buffer));
    }
}

// 绘制图像
void st7789_draw_image(const uint16_t *image_data) {
    set_address_window(0, 0, ST7789_WIDTH-1, ST7789_HEIGHT-1);

    const uint8_t *data_ptr = (const uint8_t *)image_data;
    size_t remain = ST7789_WIDTH * ST7789_HEIGHT * 2;
    
    while(remain > 0) { //  循环发送数据，每次发送不超过ST7789_MAX_TRANS_SIZE字节
        size_t send_size = (remain > ST7789_MAX_TRANS_SIZE) ? ST7789_MAX_TRANS_SIZE : remain;
        
        // 处理颜色格式转换
        uint8_t buffer[send_size];
        for(int i=0; i<send_size/2; i++) {
            uint16_t color = ((uint16_t*)data_ptr)[i];
            buffer[i*2] = ~(color >> 8);   // 高位取反
            buffer[i*2+1] = ~(color & 0xFF); // 低位取反
        }
        
        send_data(buffer, send_size);
        data_ptr += send_size;   // 指针偏移还未发送的字节数
        remain -= send_size;     // 记录还剩余的字节数
    }
}

void st7789_display_raw(const uint8_t *data, size_t len) {
    if(len != ST7789_WIDTH * ST7789_HEIGHT * 2) {
        ESP_LOGE(TAG, "数据长度不符号,当前数据长度为%d",len);
        return;
    }
    st7789_draw_image((const uint16_t *)data);
}
#include "st7789.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>

static const char *TAG = "st7789";

static spi_device_handle_t s_hspi = NULL;
static bool s_inited = false;

#if ST7789_PINGPONG_BUFFER_ENABLE
#include "esp_attr.h"

static st7789_pingpong_t s_pingpong;
static spi_transaction_t s_trans[ST7789_PINGPONG_BUF_COUNT];  // SPI事务

static void _st7789_pingpong_init(void)
{
    s_pingpong.buf_size = ST7789_PINGPONG_BUF_BYTES / sizeof(uint16_t);

    for (int i = 0; i < ST7789_PINGPONG_BUF_COUNT; i++) {
#if defined(CONFIG_GRAPHICS_USE_PSRAM)
        s_pingpong.buf[i] = heap_caps_malloc(ST7789_PINGPONG_BUF_BYTES, MALLOC_CAP_SPIRAM);
#else
        s_pingpong.buf[i] = heap_caps_malloc(ST7789_PINGPONG_BUF_BYTES, MALLOC_CAP_DMA);
#endif
        if (s_pingpong.buf[i] == NULL) {
            ESP_LOGE(TAG, "Ping-Pong 缓冲区分配失败");
            return;
        }
        s_pingpong.status[i] = PINGPONG_BUF_IDLE;
        memset(&s_trans[i], 0, sizeof(spi_transaction_t));
    }
}

// DMA 传输完成回调（ISR 上下文）
static void IRAM_ATTR _st7789_pingpong_dma_done_cb(spi_transaction_t *trans)
{
    uint8_t idx = (uint8_t)(uintptr_t)trans->user;
    s_pingpong.status[idx] = PINGPONG_BUF_IDLE;
}

static int _st7789_pingpong_get_idle_buf(void)
{
    spi_transaction_t *rtrans;
    
    while (1) {
        // 遍历查找空闲缓冲区
        for (int i = 0; i < ST7789_PINGPONG_BUF_COUNT; i++) {
            if (s_pingpong.status[i] == PINGPONG_BUF_IDLE) {
                return i;
            }
        }
        // 没有空闲的，等待 DMA 完成
        spi_device_get_trans_result(s_hspi, &rtrans, portMAX_DELAY);
    }
}

// 异步SPI发送 Ping-Pong 缓冲区(直接提交SPI事务，不阻塞)
static void _st7789_pingpong_send_async(uint8_t idx, size_t pixels)
{
    gpio_set_level(ST7789_DC_PIN, 1);
    
    s_trans[idx].length = pixels * sizeof(uint16_t) * 8;
    s_trans[idx].tx_buffer = s_pingpong.buf[idx];
    s_trans[idx].user = (void *)(uintptr_t)idx;
    
    s_pingpong.status[idx] = PINGPONG_BUF_SENDING;
    ESP_ERROR_CHECK(spi_device_queue_trans(s_hspi, &s_trans[idx], portMAX_DELAY));
}

// 等待所有 Ping-Pong 缓冲区空闲(阻塞)
static void _st7789_pingpong_wait_all_idle(void)
{
    spi_transaction_t *rtrans;
    for (int i = 0; i < ST7789_PINGPONG_BUF_COUNT; i++) {
        while (s_pingpong.status[i] == PINGPONG_BUF_SENDING) {
            spi_device_get_trans_result(s_hspi, &rtrans, portMAX_DELAY);
        }
    }
}
#endif

static void _st7789_send_cmd(uint8_t cmd) {
    gpio_set_level(ST7789_DC_PIN, 0);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(s_hspi, &t));
}

static void _st7789_send_data_polling(const uint8_t *data, size_t len)
{
    gpio_set_level(ST7789_DC_PIN, 1);
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(s_hspi, &t));
}

// 使用 DMA 方式发送大块数据
static void _st7789_send_data_dma(const uint8_t *data, size_t len)
{
    gpio_set_level(ST7789_DC_PIN, 1);
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data
    };
    ESP_ERROR_CHECK(spi_device_transmit(s_hspi, &t));
}

static void _st7789_hardware_reset(void) {
    gpio_set_level(ST7789_RES_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(ST7789_RES_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
}

static void _st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t buf[4];

    // 加上显存偏移
    x0 += ST7789_X_OFFSET;
    x1 += ST7789_X_OFFSET;
    y0 += ST7789_Y_OFFSET;
    y1 += ST7789_Y_OFFSET;

    _st7789_send_cmd(ST7789_CMD_CASET); // 列地址设置
    buf[0] = x0 >> 8; buf[1] = x0 & 0xFF;
    buf[2] = x1 >> 8; buf[3] = x1 & 0xFF;
    _st7789_send_data_polling(buf, 4);

    _st7789_send_cmd(ST7789_CMD_RASET); // 行地址设置
    buf[0] = y0 >> 8; buf[1] = y0 & 0xFF;
    buf[2] = y1 >> 8; buf[3] = y1 & 0xFF;
    _st7789_send_data_polling(buf, 4);
}

static esp_err_t _st7789_spi_bus_init(void)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = ST7789_SPI_MOSI_PIN,           // MOSI引脚
        .miso_io_num = -1,                            // 不使用MISO（显示屏不支持触屏）
        .sclk_io_num = ST7789_SPI_SCLK_PIN,           // SPI时钟引脚
        .quadwp_io_num = -1,                          // 不使用四线SPI的WP引脚
        .quadhd_io_num = -1,                          // 不使用四线SPI的HD引脚
        .max_transfer_sz = ST7789_MAX_TRANS_BYTES     // 最大传输字节数
    };

    return spi_bus_initialize(ST7789_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
}

static esp_err_t _st7789_spi_dev_init(void)
{
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = ST7789_SPI_CLOCK_HZ,        // SPI时钟频率
        .mode = ST7789_SPI_MODE,                      // SPI模式3（CPOL=1, CPHA=1）
        .spics_io_num = -1,                           // 不使用CS引脚
        .queue_size = ST7789_SPI_QUEUE_SIZE,          // SPI事务队列大小
#if ST7789_PINGPONG_BUFFER_ENABLE
        .post_cb = _st7789_pingpong_dma_done_cb,      // Ping-Pong DMA 完成回调
#endif
    };

    return spi_bus_add_device(ST7789_SPI_HOST, &devcfg, &s_hspi);
}

bool st7789_is_inited(void)
{
    return s_inited;
}

void st7789_sleep(void)
{
    if (!s_inited) return;
    _st7789_send_cmd(ST7789_CMD_SLEEP_IN);
    vTaskDelay(pdMS_TO_TICKS(5));
}

void st7789_wakeup(void)
{
    if (!s_inited) return;
    _st7789_send_cmd(ST7789_CMD_SLEEP_OUT);
    vTaskDelay(pdMS_TO_TICKS(120));
}

void st7789_display_on(void)
{
    if (!s_inited) return;
    _st7789_send_cmd(ST7789_CMD_DISPLAY_ON);
}

void st7789_display_off(void)
{
    if (!s_inited) return;
    _st7789_send_cmd(ST7789_CMD_DISPLAY_OFF);
}

static esp_err_t _st7789_config_init(void)
{
#if ST7789_PINGPONG_BUFFER_ENABLE
    _st7789_pingpong_init();                     // 初始化PINGPONG缓冲区
#endif
    
    _st7789_hardware_reset();                    // 硬件复位序列

    _st7789_send_cmd(ST7789_CMD_SLEEP_OUT); // 退出睡眠模式
    vTaskDelay(pdMS_TO_TICKS(120));

    _st7789_send_cmd(ST7789_CMD_INVON);     // 开启硬件颜色反转

    _st7789_send_cmd(ST7789_CMD_COLMOD);    // 设置颜色模式
    uint8_t colmod = ST7789_PIXEL_FORMAT;
    _st7789_send_data_polling(&colmod, 1);

    _st7789_send_cmd(ST7789_CMD_MADCTL);    // 设置内存访问方向
    uint8_t madctl = ST7789_MADCTL_RGB_V;
    _st7789_send_data_polling(&madctl, 1);

    _st7789_send_cmd(ST7789_CMD_DISPLAY_ON);// 开启显示

    return ESP_OK;
}

esp_err_t st7789_init(void)
{
    if (s_inited) {
        return ESP_OK;
    }

    esp_err_t ret = _st7789_spi_bus_init();
    if (ret != ESP_OK) return ret;

    ret = _st7789_spi_dev_init();
    if (ret != ESP_OK) return ret;

    // GPIO初始化（DC/RES引脚）
    gpio_config_t io_cfg = {
        .pin_bit_mask = (1ULL << ST7789_DC_PIN) | (1ULL << ST7789_RES_PIN),  // 配置DC和RES两个引脚
        .mode = GPIO_MODE_OUTPUT,              // 设置为输出模式
        .intr_type = GPIO_INTR_DISABLE,        // 禁用GPIO中断
        .pull_up_en = GPIO_PULLUP_DISABLE,     // 禁用内部上拉电阻
        .pull_down_en = GPIO_PULLDOWN_DISABLE  // 禁用内部下拉电阻
    };
    ret = gpio_config(&io_cfg);
    if (ret != ESP_OK) return ret;

    ret = _st7789_config_init();
    if (ret != ESP_OK) return ret;

    s_inited = true;
    ESP_LOGI(TAG, "ST7789 初始化完成");
    return ESP_OK;
}

// 绘制整屏单色(清屏)
void st7789_fill_screen(uint16_t color)
{
    if (!s_inited) return;

#if ST7789_PINGPONG_BUFFER_ENABLE
    _st7789_pingpong_wait_all_idle();
#endif

    _st7789_set_window(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
    _st7789_send_cmd(ST7789_CMD_RAMWR);

    uint16_t pixel = (color >> 8) | (color << 8);

    size_t max_pixels = ST7789_MAX_TRANS_BYTES / sizeof(uint16_t);
    uint16_t *fill_buf = (uint16_t *)heap_caps_malloc(ST7789_MAX_TRANS_BYTES, MALLOC_CAP_DMA);
    if (fill_buf == NULL) {
        ESP_LOGE(TAG, "清屏缓冲区分配失败");
        return;
    }

    for (size_t i = 0; i < max_pixels; i++) {
        fill_buf[i] = pixel;
    }

    size_t total_pixels = ST7789_WIDTH * ST7789_HEIGHT;
    size_t offset = 0;

    while (offset < total_pixels) {
        size_t pixels_left = total_pixels - offset;
        size_t send_pixels = (pixels_left > max_pixels) ? max_pixels : pixels_left;
        _st7789_send_data_dma((uint8_t *)fill_buf, send_pixels * sizeof(uint16_t));
        offset += send_pixels;
    }

    heap_caps_free(fill_buf);
}

// 绘制图像
void st7789_draw_image(const uint16_t *image_data)
{
    if (!s_inited || image_data == NULL) return;

#if ST7789_PINGPONG_BUFFER_ENABLE
    _st7789_pingpong_wait_all_idle();
#endif

    _st7789_set_window(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
    _st7789_send_cmd(ST7789_CMD_RAMWR);

#if ST7789_PINGPONG_BUFFER_ENABLE
    size_t total_pixels = ST7789_WIDTH * ST7789_HEIGHT;
    size_t offset = 0;

    while (offset < total_pixels) {
        // 获取空闲缓冲区
        int idx = _st7789_pingpong_get_idle_buf();
        
        // 标记为正在填充
        s_pingpong.status[idx] = PINGPONG_BUF_FILLING;
        
        // 计算本次拷贝像素数
        size_t pixels_left = total_pixels - offset;
        size_t copy_pixels = (pixels_left > s_pingpong.buf_size) 
                            ? s_pingpong.buf_size : pixels_left;
        
        // CPU 填充缓冲区
        memcpy(s_pingpong.buf[idx], &image_data[offset], copy_pixels * sizeof(uint16_t));
        
        // 标记为填充完成
        s_pingpong.status[idx] = PINGPONG_BUF_READY;
        
        // 异步发送（内部会设置状态为 SENDING）
        _st7789_pingpong_send_async(idx, copy_pixels);
        
        offset += copy_pixels;
    }
    
#else
    size_t max_pixels = ST7789_MAX_TRANS_BYTES / sizeof(uint16_t);
    size_t total_pixels = ST7789_WIDTH * ST7789_HEIGHT;
    size_t offset = 0;

    while (offset < total_pixels) {
        size_t pixels_left = total_pixels - offset;
        size_t send_pixels = (pixels_left > max_pixels) ? max_pixels : pixels_left;
        _st7789_send_data_dma((const uint8_t *)(image_data + offset), send_pixels * sizeof(uint16_t));
        offset += send_pixels;
    }
#endif
}

// 在指定区域绘制 RGB565 图像（适配 LVGL 刷新接口）
void st7789_draw_area(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t *color_map)
{
    if (!s_inited || color_map == NULL) return;

#if ST7789_PINGPONG_BUFFER_ENABLE
    _st7789_pingpong_wait_all_idle();
#endif

    _st7789_set_window((uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2);
    _st7789_send_cmd(ST7789_CMD_RAMWR);

    size_t max_pixels = ST7789_MAX_TRANS_BYTES / sizeof(uint16_t);
    size_t total_pixels = (size_t)(x2 - x1 + 1) * (size_t)(y2 - y1 + 1);
    size_t offset = 0;

    while (offset < total_pixels) {
        size_t pixels_left = total_pixels - offset;
        size_t send_pixels = (pixels_left > max_pixels) ? max_pixels : pixels_left;
        _st7789_send_data_dma((const uint8_t *)(color_map + offset), send_pixels * sizeof(uint16_t));
        offset += send_pixels;
    }
}

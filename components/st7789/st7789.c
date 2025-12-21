#include "st7789.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_attr.h"
#include <string.h>
#include <assert.h>

static spi_device_handle_t s_hspi = NULL;
static st7789_dma_pingpong_t s_dma_pp;
static spi_transaction_t s_dma_trans[ST7789_DMA_BUF_COUNT];
static bool s_inited = false;
static TaskHandle_t s_waiting_task = NULL;

static void _st7789_dma_pingpong_init(void);
static void _st7789_send_cmd(uint8_t cmd);
static void _st7789_send_data(const uint8_t *data, size_t len);
static void _st7789_hardware_reset(void);
static void _st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
static void _st7789_send_data_dma(uint8_t buf_idx, size_t len);

// DAM queue传输完成回调
static void IRAM_ATTR _st7789_spi_dma_done_cb(spi_transaction_t *trans)
{
    uint8_t idx = (uint8_t)(uintptr_t)trans->user;
    s_dma_pp.status[idx] = DMA_BUF_IDLE;

    TaskHandle_t t = s_waiting_task;
    if (t != NULL) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(t, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE) {
            portYIELD_FROM_ISR();
        }
    }
}

bool st7789_is_inited(void)
{
    return s_inited;
}

esp_err_t st7789_init(void)
{
    if (s_inited) {
        return ESP_OK;
    }

    // GPIO初始化（DC/RES引脚）
    gpio_config_t io_cfg = {
        .pin_bit_mask = (1ULL << ST7789_DC_PIN) | (1ULL << ST7789_RES_PIN),  // 配置DC和RES两个引脚
        .mode = GPIO_MODE_OUTPUT,              // 设置为输出模式
        .intr_type = GPIO_INTR_DISABLE,        // 禁用GPIO中断
        .pull_up_en = GPIO_PULLUP_DISABLE,     // 禁用内部上拉电阻
        .pull_down_en = GPIO_PULLDOWN_DISABLE  // 禁用内部下拉电阻
    };
    esp_err_t ret = gpio_config(&io_cfg);
    if (ret != ESP_OK) {
        return ret;
    }

    // SPI总线配置
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = ST7789_SPI_MOSI_PIN,           // 主机输出从机输入引脚（数据线）
        .miso_io_num = -1,                            // 不使用MISO（显示屏只接收数据）
        .sclk_io_num = ST7789_SPI_SCLK_PIN,           // SPI时钟引脚
        .quadwp_io_num = -1,                          // 不使用四线SPI的WP引脚
        .quadhd_io_num = -1,                          // 不使用四线SPI的HD引脚
        .max_transfer_sz = ST7789_SPI_MAX_TRANS_SIZE * 2  // 最大传输字节数（每个像素2字节）
    };

    // SPI设备配置
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = ST7789_SPI_CLOCK_HZ,   // SPI时钟频率40MHz
        .mode = ST7789_SPI_MODE,                 // SPI模式:3（CPOL=1, CPHA=1）
        .spics_io_num = -1,                      // 不使用CS引脚
        .queue_size = ST7789_SPI_QUEUE_SIZE,     // SPI事务队列大小
        .flags = SPI_DEVICE_HALFDUPLEX,          // 半双工，只用mosi
        .post_cb = _st7789_spi_dma_done_cb,      // 回调
    };

    ret = spi_bus_initialize(ST7789_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);// 初始化SPI总线
    if (ret != ESP_OK) {
        return ret;
    }
    ret = spi_bus_add_device(ST7789_SPI_HOST, &dev_cfg, &s_hspi);// 初始化SPI设备
    if (ret != ESP_OK) {
        return ret;
    }

    _st7789_dma_pingpong_init();         // 初始化缓冲区
    
    _st7789_hardware_reset();               // 硬件复位序列

    _st7789_send_cmd(ST7789_CMD_SLEEP_OUT); // 退出睡眠模式
    vTaskDelay(pdMS_TO_TICKS(120));

    _st7789_send_cmd(ST7789_CMD_INVON);     // 开启硬件颜色反转

    _st7789_send_cmd(ST7789_CMD_COLMOD);    // 设置颜色模式
    uint8_t colmod = ST7789_PIXEL_FORMAT;
    _st7789_send_data(&colmod, 1);

    _st7789_send_cmd(ST7789_CMD_MADCTL);    // 设置内存访问方向
    uint8_t madctl = ST7789_MADCTL_RGB_V;
    _st7789_send_data(&madctl, 1);

    _st7789_send_cmd(ST7789_CMD_DISPLAY_ON);// 开启显示

    s_inited = true;
    return ESP_OK;
}

static void _st7789_dma_pingpong_init(void)
{
    s_dma_pp.buf_size = ST7789_MAX_TRANS_SIZE / 2;

    for (int i = 0; i < ST7789_DMA_BUF_COUNT; i++) {
        s_dma_pp.buf[i] = heap_caps_malloc(
            ST7789_MAX_TRANS_SIZE,
            MALLOC_CAP_DMA
        );
        assert(s_dma_pp.buf[i] != NULL);

        s_dma_pp.status[i] = DMA_BUF_IDLE;
        memset(&s_dma_trans[i], 0, sizeof(s_dma_trans[i]));
    }

    s_dma_pp.cpu_idx = 0;
    s_dma_pp.dma_idx = 0;
}

// 发送命令
static void _st7789_send_cmd(uint8_t cmd) {
    gpio_set_level(ST7789_DC_PIN, 0);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(s_hspi, &t));
}

// 发送命令数据
static void _st7789_send_data(const uint8_t *data, size_t len)
{
    gpio_set_level(ST7789_DC_PIN, 1);
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(s_hspi, &t));
}

// 硬件复位
static void _st7789_hardware_reset(void) {
    // 将ST7789_RES_PIN引脚电平设置为0，即低电平
    gpio_set_level(ST7789_RES_PIN, 0);
    // 延时20毫秒
    vTaskDelay(pdMS_TO_TICKS(20));
    // 将ST7789_RES_PIN引脚电平设置为1，即高电平
    gpio_set_level(ST7789_RES_PIN, 1);
    // 延时120毫秒
    vTaskDelay(pdMS_TO_TICKS(120));
}

// 设置绘图窗口
static void _st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t buf[4];

    _st7789_send_cmd(ST7789_CMD_CASET);
    buf[0] = x0 >> 8; buf[1] = x0 & 0xFF;
    buf[2] = x1 >> 8; buf[3] = x1 & 0xFF;
    _st7789_send_data(buf, 4);

    _st7789_send_cmd(ST7789_CMD_RASET);
    buf[0] = y0 >> 8; buf[1] = y0 & 0xFF;
    buf[2] = y1 >> 8; buf[3] = y1 & 0xFF;
    _st7789_send_data(buf, 4);
}

static void _st7789_send_data_dma(uint8_t buf_idx, size_t len)
{
    gpio_set_level(ST7789_DC_PIN, 1);

    if (buf_idx >= ST7789_DMA_BUF_COUNT) {
        return;
    }

    while (s_dma_pp.status[buf_idx] != DMA_BUF_READY) {
        taskYIELD();
    }

    s_dma_pp.status[buf_idx] = DMA_BUF_SENDING;

    spi_transaction_t *t = &s_dma_trans[buf_idx];
    memset(t, 0, sizeof(*t));

    t->length    = len * 8;
    t->tx_buffer = s_dma_pp.buf[buf_idx];
    t->user      = (void *)(uintptr_t)buf_idx;

    spi_transaction_t *rtrans = NULL;
    while (spi_device_get_trans_result(s_hspi, &rtrans, 0) == ESP_OK) {
    }

    ESP_ERROR_CHECK(
        spi_device_queue_trans(
            s_hspi,
            t,
            portMAX_DELAY
        )
    );
}

// 绘制整屏单色(清屏)
void st7789_fill_screen(uint16_t color)
{
    _st7789_set_window(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
    _st7789_send_cmd(ST7789_CMD_RAMWR);

    uint16_t pixel = (color >> 8) | (color << 8);

    uint16_t line_buf[ST7789_WIDTH];

    for (int x = 0; x < ST7789_WIDTH; x++) {
        line_buf[x] = pixel;
    }

    for (int y = 0; y < ST7789_HEIGHT; y++) {
        _st7789_send_data((uint8_t *)line_buf, ST7789_WIDTH * 2);
    }
}

// 绘像
void st7789_draw_image(const uint16_t *image_data)
{
    _st7789_set_window(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
    _st7789_send_cmd(ST7789_CMD_RAMWR);

    size_t total_pixels = ST7789_WIDTH * ST7789_HEIGHT;
    size_t offset = 0;

    s_waiting_task = xTaskGetCurrentTaskHandle(); // 获取当前任务句柄
    (void)ulTaskNotifyTake(pdTRUE, 0);

    while (offset < total_pixels) {

        uint8_t idx = s_dma_pp.cpu_idx;

        while (s_dma_pp.status[idx] != DMA_BUF_IDLE) {
            (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }

        s_dma_pp.status[idx] = DMA_BUF_FILLING;

        size_t pixels_left = total_pixels - offset;
        size_t copy_pixels = pixels_left > s_dma_pp.buf_size
                            ? s_dma_pp.buf_size
                            : pixels_left;

        memcpy(
            s_dma_pp.buf[idx],
            &image_data[offset],
            copy_pixels * sizeof(uint16_t)
        );

        s_dma_pp.status[idx] = DMA_BUF_READY;

        _st7789_send_data_dma(
            idx,
            copy_pixels * sizeof(uint16_t)
        );

        offset += copy_pixels;
        s_dma_pp.cpu_idx ^= 1;
    }

    s_waiting_task = NULL;
}

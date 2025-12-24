#include "st7789.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_attr.h"
#include <string.h>

static spi_device_handle_t s_hspi = NULL;
#if ST7789_PINGPONG_BUFFER_ENABLE
static st7789_pingpong_t s_pingpong;
static spi_transaction_t s_pingpong_trans[ST7789_PINGPONG_BUF_COUNT];
static TaskHandle_t s_waiting_task = NULL;
#endif
static bool s_inited = false;

#if ST7789_PINGPONG_BUFFER_ENABLE
static void _st7789_pingpong_init(void)
{
    s_pingpong.buf_size = ST7789_DMA_MAX_PIXELS;

    for (int i = 0; i < ST7789_PINGPONG_BUF_COUNT; i++) {
#if defined(CONFIG_GRAPHICS_USE_PSRAM)
        s_pingpong.buf[i] = heap_caps_malloc(s_pingpong.buf_size * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
#else
        s_pingpong.buf[i] = heap_caps_malloc(s_pingpong.buf_size * sizeof(uint16_t), MALLOC_CAP_DMA);
#endif
        if (s_pingpong.buf[i] == NULL) {
            ESP_LOGE("ST7789", "pingpong buffer malloc failed");
            return;
        }
        s_pingpong.status[i] = PINGPONG_BUF_IDLE;
        memset(&s_pingpong_trans[i], 0, sizeof(s_pingpong_trans[i]));
    }
    s_pingpong.cpu_idx = 0;
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

#if ST7789_PINGPONG_BUFFER_ENABLE
static void _st7789_send_data_queue(const uint8_t *data, size_t len, void *user_data)
{
    gpio_set_level(ST7789_DC_PIN, 1);

    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,  
        .user = user_data
    
    };

    spi_transaction_t *rtrans = NULL;
    while (spi_device_get_trans_result(s_hspi, &rtrans, 0) == ESP_OK) {}  // 等待前一个事务完成(非阻塞)

    ESP_ERROR_CHECK(spi_device_queue_trans(s_hspi, &t, portMAX_DELAY));
    ESP_ERROR_CHECK(spi_device_get_trans_result(s_hspi, &rtrans, portMAX_DELAY)); // 等待当前事务完成(阻塞)
}
#endif

static void _st7789_hardware_reset(void) {
    gpio_set_level(ST7789_RES_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(ST7789_RES_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
}

static void _st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t buf[4];

    _st7789_send_cmd(ST7789_CMD_CASET); // 列地址设置
    buf[0] = x0 >> 8; buf[1] = x0 & 0xFF;
    buf[2] = x1 >> 8; buf[3] = x1 & 0xFF;
    _st7789_send_data_polling(buf, 4);

    _st7789_send_cmd(ST7789_CMD_RASET); // 行地址设置
    buf[0] = y0 >> 8; buf[1] = y0 & 0xFF;
    buf[2] = y1 >> 8; buf[3] = y1 & 0xFF;
    _st7789_send_data_polling(buf, 4);
}

#if ST7789_PINGPONG_BUFFER_ENABLE
static void IRAM_ATTR _st7789_spi_dma_done_cb(spi_transaction_t *trans)
{
    uint8_t idx = (uint8_t)(uintptr_t)trans->user;
    s_pingpong.status[idx] = PINGPONG_BUF_IDLE;

    TaskHandle_t t = s_waiting_task;
    if (t != NULL) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(t, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE) {
            portYIELD_FROM_ISR();
        }
    }
}
#endif

static esp_err_t _st7789_spi_bus_init(void)
{
    // SPI总线配置
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = ST7789_SPI_MOSI_PIN,           // 主机输出从机输入引脚（数据线）
        .miso_io_num = -1,                            // 不使用MISO（显示屏只接收数据）
        .sclk_io_num = ST7789_SPI_SCLK_PIN,           // SPI时钟引脚
        .quadwp_io_num = -1,                          // 不使用四线SPI的WP引脚
        .quadhd_io_num = -1,                          // 不使用四线SPI的HD引脚
        .max_transfer_sz = ST7789_SPI_MAX_TRANS_SIZE  // 最大传输字节数（单位：字节）
    };

    ESP_ERROR_CHECK(spi_bus_initialize(ST7789_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    // SPI设备配置
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = ST7789_SPI_CLOCK_HZ,   // SPI时钟频率
        .mode = ST7789_SPI_MODE,                 // SPI模式:3（CPOL=1, CPHA=1）
        .spics_io_num = -1,                      // 不使用CS引脚
        .queue_size = ST7789_SPI_QUEUE_SIZE,     // SPI事务队列大小
#if ST7789_PINGPONG_BUFFER_ENABLE
        .post_cb = _st7789_spi_dma_done_cb,      // DMA传输完成中断处理函数
#else
        .post_cb = NULL,
#endif
    };

    ESP_ERROR_CHECK(spi_bus_add_device(ST7789_SPI_HOST, &dev_cfg, &s_hspi));

    return ESP_OK;
}

bool st7789_is_inited(void)
{
    return s_inited;
}

static esp_err_t _st7789_config_init(void)
{
#if ST7789_PINGPONG_BUFFER_ENABLE
    _st7789_pingpong_init();            // 初始化缓冲区
#endif
    
    _st7789_hardware_reset();               // 硬件复位序列

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
    ESP_ERROR_CHECK(_st7789_spi_bus_init());

    // GPIO初始化（DC/RES引脚）
    gpio_config_t io_cfg = {
        .pin_bit_mask = (1ULL << ST7789_DC_PIN) | (1ULL << ST7789_RES_PIN),  // 配置DC和RES两个引脚
        .mode = GPIO_MODE_OUTPUT,              // 设置为输出模式
        .intr_type = GPIO_INTR_DISABLE,        // 禁用GPIO中断
        .pull_up_en = GPIO_PULLUP_DISABLE,     // 禁用内部上拉电阻
        .pull_down_en = GPIO_PULLDOWN_DISABLE  // 禁用内部下拉电阻
    };
    ESP_ERROR_CHECK(gpio_config(&io_cfg));

    ESP_ERROR_CHECK(_st7789_config_init());

    s_inited = true;
    return ESP_OK;
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
        _st7789_send_data_polling((uint8_t *)line_buf, ST7789_WIDTH * 2);
    }
}

// 绘像
void st7789_draw_image(const uint16_t *image_data)
{
    _st7789_set_window(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
    _st7789_send_cmd(ST7789_CMD_RAMWR);

#if ST7789_PINGPONG_BUFFER_ENABLE
    size_t total_pixels = ST7789_WIDTH * ST7789_HEIGHT;
    size_t offset = 0;

    s_waiting_task = xTaskGetCurrentTaskHandle(); // 获取当前任务句柄
    (void)ulTaskNotifyTake(pdTRUE, 0);

    while (offset < total_pixels) {

        uint8_t idx = s_pingpong.cpu_idx;

        while (s_pingpong.status[idx] != PINGPONG_BUF_IDLE) {
            (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }

        s_pingpong.status[idx] = PINGPONG_BUF_FILLING;

        size_t pixels_left = total_pixels - offset;
        size_t copy_pixels = pixels_left > s_pingpong.buf_size
                            ? s_pingpong.buf_size
                            : pixels_left;

        memcpy(
            s_pingpong.buf[idx],
            &image_data[offset],
            copy_pixels * sizeof(uint16_t)
        );

        s_pingpong.status[idx] = PINGPONG_BUF_READY;

        _st7789_send_data_queue(
            (const uint8_t *)s_pingpong.buf[idx],
            copy_pixels * sizeof(uint16_t),
            (void *)(uintptr_t)idx
        );

        offset += copy_pixels;
        s_pingpong.cpu_idx ^= 1;
    }

    s_waiting_task = NULL;
#else
    size_t total_pixels = ST7789_WIDTH * ST7789_HEIGHT;
    size_t offset = 0;

    while (offset < total_pixels) {
        size_t pixels_left = total_pixels - offset;

        // 每次发送不超过 ST7789_SPI_MAX_TRANS_SIZE 字节（polling 直接发）
        size_t max_bytes  = ST7789_SPI_MAX_TRANS_SIZE;
        size_t max_pixels = max_bytes / sizeof(uint16_t);

        size_t copy_pixels = (pixels_left > max_pixels) ? max_pixels : pixels_left;
        size_t copy_bytes  = copy_pixels * sizeof(uint16_t);

        _st7789_send_data_polling((const uint8_t *)(image_data + offset), copy_bytes);
        offset += copy_pixels;
    }
#endif
}

// 在指定区域绘制 RGB565 图像（适配 LVGL 刷新接口）
void st7789_draw_area(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t *color_map)
{
    _st7789_set_window((uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2);
    _st7789_send_cmd(ST7789_CMD_RAMWR);

    uint32_t w = (uint32_t)(x2 - x1 + 1);
    uint32_t h = (uint32_t)(y2 - y1 + 1);
    uint32_t total_pixels = w * h;
    uint32_t offset = 0;

    while (offset < total_pixels) {
        uint32_t pixels_left = total_pixels - offset;

        // 每次发送不超过 ST7789_SPI_MAX_TRANS_SIZE 字节（polling 直接发）
        uint32_t max_bytes  = ST7789_SPI_MAX_TRANS_SIZE;
        uint32_t max_pixels = max_bytes / sizeof(uint16_t);

        uint32_t copy_pixels = (pixels_left > max_pixels) ? max_pixels : pixels_left;
        uint32_t copy_bytes  = copy_pixels * sizeof(uint16_t);

        _st7789_send_data_polling((const uint8_t *)(color_map + offset), copy_bytes);
        offset += copy_pixels;
    }
}
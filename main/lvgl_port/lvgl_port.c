
#include "lvgl_port.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "st7789.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#ifndef MY_DISP_HOR_RES
    #define MY_DISP_HOR_RES    ST7789_WIDTH
#endif

#ifndef MY_DISP_VER_RES
    #define MY_DISP_VER_RES    ST7789_HEIGHT
#endif

/**
 * @brief       lvgl_demo入口函数
 * @param       无
 * @retval      无
 */
void lvgl_demo(void)
{
    lv_init();              /* 初始化LVGL图形库 */
    lv_port_disp_init();    /* lvgl显示接口初始化,放在lv_init()的后面 */
    //lv_port_indev_init();   /* lvgl输入接口初始化,放在lv_init()的后面 */

    /* 为LVGL提供时基单元 */
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &increase_lvgl_tick,    /* 设置定时器回调 */
        .name = "lvgl_tick"                 /* 定时器名称 */
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));     /* 创建定时器 */
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 1 * 1000));           /* 启动定时器 */

    /* 简单示例界面：创建一个标签，显示在屏幕中心 */
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello LVGL");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    while (1)
    {
        lv_timer_handler();             /* LVGL计时器 */
        vTaskDelay(pdMS_TO_TICKS(10));  /* 延时10毫秒 */
    }
}

/**
 * @brief       初始化并注册显示设备
 * @param       无
 * @retval      lvgl显示设备指针
 */
lv_disp_t *lv_port_disp_init(void)
{
    // void *lcd_buffer[2];        /* 指向屏幕双缓存 */
    
    /* 初始化显示设备 LCD */
    st7789_init();

    /*-----------------------------
     * 创建一个绘图缓冲区
     *----------------------------*/
    /**
     * LVGL 需要一个缓冲区用来绘制小部件
     * 随后，这个缓冲区的内容会通过显示设备的 'flush_cb'(显示设备刷新函数) 复制到显示设备上
     * 这个缓冲区的大小需要大于显示设备一行的大小
     *
     * 这里有3种缓冲配置:
     * 1. 单缓冲区:
     *      LVGL 会将显示设备的内容绘制到这里，并将他写入显示设备。
     *
     * 2. 双缓冲区:
     *      LVGL 会将显示设备的内容绘制到其中一个缓冲区，并将他写入显示设备。
     *      需要使用 DMA 将要显示在显示设备的内容写入缓冲区。
     *      当数据从第一个缓冲区发送时，它将使 LVGL 能够将屏幕的下一部分绘制到另一个缓冲区。
     *      这样使得渲染和刷新可以并行执行。
     *
     * 3. 全尺寸双缓冲区
     *      设置两个屏幕大小的全尺寸缓冲区，并且设置 disp_drv.full_refresh = 1。
     *      这样，LVGL将始终以 'flush_cb' 的形式提供整个渲染屏幕，您只需更改帧缓冲区的地址。
     */
    /* 使用双缓冲 */
    
    // 缓冲区大小：屏幕 1/10 行数，可根据需要调整
    int DISP_BUF_SIZE = MY_DISP_HOR_RES * ((MY_DISP_VER_RES) / 10);

    static lv_disp_draw_buf_t draw_buf_dsc;
#if defined(CONFIG_GRAPHICS_USE_PSRAM)
    /* 有 PSRAM 时，把 LVGL 显存放到 PSRAM，减轻内部 RAM 压力 */
    lv_color_t *buf_1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    lv_color_t *buf_2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
#else
    lv_color_t *buf_1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_color_t *buf_2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
#endif
    if (buf_1 == NULL || buf_2 == NULL) {
        ESP_LOGE("LVGL", "Failed to allocate display buffers!");
        return NULL;
    }
    lv_disp_draw_buf_init(&draw_buf_dsc, buf_1, buf_2, DISP_BUF_SIZE);


    /* 在LVGL中注册显示设备 */
    static lv_disp_drv_t disp_drv;      /* 显示设备的描述符(HAL要注册的显示驱动程序、与显示交互并处理与图形相关的结构体、回调函数) */
    lv_disp_drv_init(&disp_drv);        /* 初始化显示设备 */
    
    /* 设置显示设备的分辨率 */
    disp_drv.hor_res = ST7789_WIDTH;                    /* 设置水平分辨率 */
    disp_drv.ver_res = ST7789_HEIGHT;                   /* 设置垂直分辨率 */

    /* 用来将缓冲区的内容复制到显示设备 */
    disp_drv.flush_cb = lvgl_disp_flush_cb;             /* 设置刷新回调函数 */

    /* 设置显示缓冲区 */
    disp_drv.draw_buf = &draw_buf_dsc;                      /* 设置绘画缓冲区 */

    disp_drv.user_data = NULL;
    disp_drv.full_refresh = 1;                          /* 设置为完全刷新 */
    /* 注册显示设备 */
    return lv_disp_drv_register(&disp_drv);                    
}

/**
* @brief        将内部缓冲区的内容刷新到显示屏上的特定区域
* @note         可以使用 DMA 或者任何硬件在后台加速执行这个操作
*               但是，需要在刷新完成后调用函数 'lv_disp_flush_ready()'
* @param        disp_drv : 显示设备
* @param        area : 要刷新的区域，包含了填充矩形的对角坐标
* @param        color_map : 颜色数组
* @retval       无
*/
void lvgl_disp_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    /* 将 LVGL 提供的区域刷新到 ST7789 显示屏 */
    st7789_draw_area(area->x1, area->y1, area->x2, area->y2, (const uint16_t *)color_map);

    /* 重要!!! 通知图形库，已经刷新完毕了 */
    lv_disp_flush_ready(drv);
}

/**
 * @brief       告诉LVGL运行时间
 * @param       arg : 传入参数(未用到)
 * @retval      无
 */
void increase_lvgl_tick(void *arg)
{
    /* 告诉LVGL已经过了多少毫秒 */
    lv_tick_inc(1);
}
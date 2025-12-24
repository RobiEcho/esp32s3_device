/**
 * @file lv_port_disp_templ.c
 *
 */

/* 将此文件复制为 "lv_port_disp.c" 并将下面的值设为 "1" 以启用内容 */
#if 1

/*********************
 *      包含头文件
 *********************/
#include "lv_port_disp.h"
#include <stdbool.h>
#include "st7789.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

/*********************
 *      宏定义
 *********************/
#ifndef MY_DISP_HOR_RES
    #warning 请定义或替换宏 MY_DISP_HOR_RES 为实际屏幕宽度，当前使用默认值 320。
    #define MY_DISP_HOR_RES    ST7789_WIDTH
#endif

#ifndef MY_DISP_VER_RES
    #warning 请定义或替换宏 MY_DISP_VER_RES 为实际屏幕高度，当前使用默认值 240。
    #define MY_DISP_VER_RES    ST7789_HEIGHT
#endif

/**********************
 *      类型定义
 **********************/

/**********************
 *  静态函数声明
 **********************/
static void disp_init(void);

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//        const lv_area_t * fill_area, lv_color_t color);

/**********************
 *  静态变量
 **********************/

/**********************
 *      宏
 **********************/

/**********************
 *   全局函数
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * 初始化你的显示屏
     * -----------------------*/
    disp_init();

    /*-----------------------------
     * 创建用于绘图的缓冲区
     *----------------------------*/

    /**
     * LVGL 需要一个缓冲区，用于在其内部绘制控件。
     * 之后，该缓冲区的内容会通过你的显示驱动的 `flush_cb` 回调函数复制到显示屏上。
     * 缓冲区大小必须大于一行像素。
     *
     * 有三种缓冲配置方式：
     * 1. 创建单个缓冲区：
     *      LVGL 在此缓冲区中绘制内容，然后将其写入显示屏。
     *
     * 2. 创建两个缓冲区：
     *      LVGL 向一个缓冲区绘制内容并写入显示屏。
     *      你可以使用 DMA 将缓冲区内容发送到显示屏。
     *      这样，在第一个缓冲区的数据正在被发送的同时，
     *      LVGL 可以向第二个缓冲区绘制下一部分画面，实现渲染与刷新并行。
     *
     * 3. 双缓冲（全屏刷新）：
     *      设置两个与屏幕尺寸相同的缓冲区，并设置 disp_drv.full_refresh = 1。
     *      此时 LVGL 会在每次 `flush_cb` 调用时提供完整的渲染画面，
     *      你只需切换帧缓冲区的地址即可。
     */

    /* 示例 1：单缓冲区 */
    // static lv_disp_draw_buf_t draw_buf_dsc_1;
    // static lv_color_t buf_1[MY_DISP_HOR_RES * 10];                          /* 10 行像素的缓冲区 */
    // lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, MY_DISP_HOR_RES * 10);   /* 初始化显示缓冲区 */

    /* 示例 2：双缓冲区（非全屏） */
    // static lv_disp_draw_buf_t draw_buf_dsc_2;
    // static lv_color_t buf_2_1[MY_DISP_HOR_RES * 10];                        /* 第一个 10 行缓冲区 */
    // static lv_color_t buf_2_2[MY_DISP_HOR_RES * 10];                        /* 第二个 10 行缓冲区 */
    // lv_disp_draw_buf_init(&draw_buf_dsc_2, buf_2_1, buf_2_2, MY_DISP_HOR_RES * 10);   /* 初始化显示缓冲区 */

    /* 示例 3：双缓冲（全屏），需同时设置 disp_drv.full_refresh = 1（见下方） */
    // static lv_disp_draw_buf_t draw_buf_dsc_3;
    // static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /* 一个完整屏幕大小的缓冲区 */
    // static lv_color_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /* 另一个完整屏幕大小的缓冲区 */
    // lv_disp_draw_buf_init(&draw_buf_dsc_3, buf_3_1, buf_3_2,
    //                       MY_DISP_VER_RES * LV_VER_RES_MAX);   /* 初始化显示缓冲区 */
    /* 使用双缓冲区 */
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
        return;
    }
    lv_disp_draw_buf_init(&draw_buf_dsc, buf_1, buf_2, DISP_BUF_SIZE);

    /*-----------------------------------
     * 在 LVGL 中注册显示驱动
     *----------------------------------*/

    static lv_disp_drv_t disp_drv;                         /* 显示驱动描述符 */
    lv_disp_drv_init(&disp_drv);                    /* 基本初始化 */

    /* 设置访问显示屏所需的回调函数 */

    /* 设置显示屏分辨率 */
    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;

    /* 设置刷新回调函数，用于将缓冲区内容复制到显示屏 */
    disp_drv.flush_cb = disp_flush;

    /* 设置显示缓冲区（这里使用示例 2 的双缓冲区） */
    disp_drv.draw_buf = &draw_buf_dsc;

    disp_drv.user_data = NULL;
    disp_drv.full_refresh = 1;                          /* 设置为完全刷新 */

    /* 若使用示例 3（全屏双缓冲），需取消下面这行注释 */
    //disp_drv.full_refresh = 1;

    /* 如果你的 MCU 有 GPU，可在此设置 GPU 填充回调。
     * 注意：在 lv_conf.h 中可以启用 LVGL 内置支持的 GPU。
     * 如果你使用的是其他 GPU，可通过此回调进行集成。*/
    //disp_drv.gpu_fill_cb = gpu_fill;

    /* 最后，注册该显示驱动 */
    lv_disp_drv_register(&disp_drv);
}

/**********************
 *   静态函数
 **********************/

/* 初始化你的显示屏及相关外设 */
static void disp_init(void)
{
    /* 在此处添加你的初始化代码 */
    st7789_init();
}

/* 全局标志：控制是否允许刷新屏幕 */
volatile bool disp_flush_enabled = true;

/* 启用屏幕更新（即允许在 LVGL 调用 disp_flush() 时执行刷新操作） */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* 禁用屏幕更新（即阻止在 LVGL 调用 disp_flush() 时执行刷新操作） */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/* 将内部缓冲区中指定区域的内容刷新到显示屏上
 * 你可以使用 DMA 或硬件加速在后台完成此操作，
 * 但操作完成后必须调用 `lv_disp_flush_ready()` 通知 LVGL。*/
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    // if(disp_flush_enabled) {
    //     /* 最简单的实现方式（但速度最慢）：逐个像素写入显示屏 */

    //     int32_t x;
    //     int32_t y;
    //     for(y = area->y1; y <= area->y2; y++) {
    //         for(x = area->x1; x <= area->x2; x++) {
    //             /* 向显示屏写入一个像素。例如：*/
    //             /* put_px(x, y, *color_p); */
    //             color_p++;
    //         }
    //     }
    // }

    st7789_draw_area(area->x1, area->y1, area->x2, area->y2, (const uint16_t *)color_p);

    /* 重要！！！
     * 必须通知图形库：刷新操作已完成 */
    lv_disp_flush_ready(disp_drv);
}

/*【可选】GPU 接口 */

/* 如果你的 MCU 配备了硬件加速器（GPU），可使用它来快速填充内存区域 */
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//                    const lv_area_t * fill_area, lv_color_t color)
//{
//    /* 以下仅为示例代码，实际应由你的 GPU 完成 */
//    int32_t x, y;
//    dest_buf += dest_width * fill_area->y1; /* 定位到第一行 */
//
//    for(y = fill_area->y1; y <= fill_area->y2; y++) {
//        for(x = fill_area->x1; x <= fill_area->x2; x++) {
//            dest_buf[x] = color;
//        }
//        dest_buf += dest_width;    /* 移动到下一行 */
//    }
//}


#else /* 若未启用此文件（顶部 #if 为 0）*/

/* 此 dummy typedef 仅用于避免 -Wpedantic 警告 */
typedef int keep_pedantic_happy;
#endif
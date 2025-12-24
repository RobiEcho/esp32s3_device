/**
 * @file lv_port_disp_templ.h
 *
 */

/*将此文件复制为 "lv_port_disp.h" 并将此值设置为 "1" 以启用内容*/
#if 1

#ifndef LV_PORT_DISP_TEMPL_H
#define LV_PORT_DISP_TEMPL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      包含文件
 *********************/
#include "lvgl.h"

/*********************
 *      定义
 *********************/

/**********************
 *      类型定义
 **********************/

/**********************
 * 全局函数原型
 **********************/
/* 初始化低级显示驱动程序 */
void lv_port_disp_init(void);

/* 当 LVGL 调用 disp_flush() 时启用更新屏幕（刷新过程） */
void disp_enable_update(void);

/* 当 LVGL 调用 disp_flush() 时禁用更新屏幕（刷新过程） */
void disp_disable_update(void);

/**********************
 *      宏
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PORT_DISP_TEMPL_H*/

#endif /*禁用/启用内容*/

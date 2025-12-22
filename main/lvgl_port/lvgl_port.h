#ifndef __LVGL_PORT_H
#define __LVGL_PORT_H

#include "lvgl.h"

void lvgl_demo(void);                                                                               /* lvgl_demo入口函数 */
lv_disp_t *lv_port_disp_init(void);                                                                 /* 初始化并注册显示设备 */
void increase_lvgl_tick(void *arg);                                                          /* 告诉LVGL运行时间 */
void lvgl_disp_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);   /* 将内部缓冲区的内容刷新到显示屏上的特定区域 */

#endif

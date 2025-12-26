#ifndef __LVGL_UI_H__
#define __LVGL_UI_H__

#include "lvgl.h"
#include "esp_err.h"

/**
 * @brief 配置LVGL时钟（定时器）
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t lvgl_ui_tick(void);

/**
 * @brief 创建UI界面
 */
void lvgl_ui_create(void);


void test_ui_create(void);

#endif // __LVGL_UI_H__

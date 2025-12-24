#ifndef __LVGL_UI_CONFIG_H__
#define __LVGL_UI_CONFIG_H__

#include "lvgl.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 配置LVGL时钟（定时器）
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t lvgl_ui_config_tick(void);

/**
 * @brief 创建UI界面
 */
void lvgl_ui_create(void);

#ifdef __cplusplus
}
#endif

#endif // __LVGL_UI_CONFIG_H__


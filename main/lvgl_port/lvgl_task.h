#ifndef __LVGL_TASK_H__
#define __LVGL_TASK_H__

#include "esp_err.h"

/* LVGL任务配置 */
#define LVGL_TASK_STACK_SIZE    (8192)
#define LVGL_TASK_PRIORITY      (5)
#define LVGL_TASK_PERIOD_MS     (10)

/**
 * @brief 初始化并启动LVGL任务
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t lvgl_task_init(void);

#endif // __LVGL_TASK_H__

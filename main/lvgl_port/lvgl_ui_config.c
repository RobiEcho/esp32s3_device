#include "lvgl_ui_config.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_err.h"
#include "lvgl.h"

static const char *TAG = "LVGL_UI_CONFIG";

/* LVGL时钟定时器句柄 */
static esp_timer_handle_t s_lvgl_tick_timer = NULL;

/**
 * @brief LVGL时钟回调函数
 */
static void lvgl_tick_cb(void *arg)
{
    lv_tick_inc(1);
}

/**
 * @brief 配置LVGL时钟（定时器）
 */
esp_err_t lvgl_ui_config_tick(void)
{
    if (s_lvgl_tick_timer != NULL) {
        ESP_LOGW(TAG, "LVGL tick timer already configured");
        return ESP_OK;
    }

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = lvgl_tick_cb,  // 回调函数
        .name = "lvgl_tick"        // 定时器名称
    };

    esp_err_t ret = esp_timer_create(&lvgl_tick_timer_args, &s_lvgl_tick_timer); // 创建定时器
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LVGL tick timer");
        return ret;
    }

    ret = esp_timer_start_periodic(s_lvgl_tick_timer, 1 * 1000); // 启动定时器，周期为1ms
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start LVGL tick timer");
        esp_timer_delete(s_lvgl_tick_timer);
        s_lvgl_tick_timer = NULL;
        return ret;
    }

    ESP_LOGI(TAG, "LVGL tick timer configured");
    return ESP_OK;
}

/**
 * @brief 创建UI界面
 */
void lvgl_ui_create(void)
{
    /* 简单示例界面：创建一个标签，显示在屏幕中心 */
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello LVGL");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    ESP_LOGI(TAG, "UI created");
}


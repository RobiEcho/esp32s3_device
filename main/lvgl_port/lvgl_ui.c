#include "lvgl_ui.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_err.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "lvgl_ui";

/* LVGL时钟定时器句柄 */
static esp_timer_handle_t s_lvgl_tick_timer = NULL;

/* 动画配置 */
#define ANIM_OBJ_SIZE           40
#define ANIM_X_MIN              0
#define ANIM_X_MAX              200
#define ANIM_Y_POS              100
#define ANIM_DURATION_MS        3000

/* 动画对象指针 */
static lv_obj_t *anim_obj = NULL;

/* 加载圆圈UI相关变量 */
static lv_obj_t *s_arc = NULL;
static lv_obj_t *s_arc_label = NULL;
static lv_obj_t *s_title_label = NULL;
static lv_timer_t *s_arc_timer = NULL;
static int32_t s_arc_value = 0;

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
esp_err_t lvgl_ui_tick(void)
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
 * @brief 圆弧加载定时器回调函数
 */
static void arc_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    
    if (!s_arc || !s_arc_label) return;
    
    /* 每秒增加5% */
    s_arc_value += 5;
    
    /* 限制在100% */
    if (s_arc_value > 100) {
        s_arc_value = 100;
    }
    
    /* 更新圆弧值 */
    lv_arc_set_value(s_arc, s_arc_value);
    
    /* 更新标签文字 */
    if (s_arc_value >= 100) {
        /* 达到100%时，修改标题文字为Hello World! */
        if (s_title_label) {
            lv_label_set_text(s_title_label, "Hello World!");
        }
        /* 更新圆弧标签为100% */
        if (s_arc_label) {
            lv_label_set_text(s_arc_label, "100%");
        }
        /* 停止定时器 */
        if (s_arc_timer) {
            lv_timer_del(s_arc_timer);
            s_arc_timer = NULL;
        }
    } else {
        /* 显示百分比 */
        char buf[8];
        lv_snprintf(buf, sizeof(buf), "%d%%", (int)s_arc_value);
        lv_label_set_text(s_arc_label, buf);
    }
}

/**
 * @brief 创建UI界面
 */
void lvgl_ui_create(void)
{
    /* 设置屏幕背景色为粉红色 */
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xFFB6C1), LV_PART_MAIN);
    
    /* 1. 创建标题文字 - 中间靠上 */
    s_title_label = lv_label_create(lv_scr_act());
    lv_label_set_text(s_title_label, "ST7789 LVGL");
    lv_obj_set_style_text_color(s_title_label, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_title_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(s_title_label, LV_ALIGN_CENTER, 0, -50);
    
    /* 2. 创建圆弧（加载圆圈）- 中间靠下 */
    s_arc = lv_arc_create(lv_scr_act());
    lv_obj_set_size(s_arc, 80, 80);
    lv_obj_align(s_arc, LV_ALIGN_CENTER, 0, 40);
    lv_arc_set_range(s_arc, 0, 100);
    lv_arc_set_value(s_arc, 0);
    lv_arc_set_bg_angles(s_arc, 0, 360);
    lv_obj_set_style_arc_color(s_arc, lv_color_hex(0xD3D3D3), LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_arc, lv_color_hex(0x4169E1), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_arc, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_arc, 8, LV_PART_INDICATOR);
    lv_obj_remove_style(s_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(s_arc, LV_OBJ_FLAG_CLICKABLE);
    
    /* 圆弧中心文字 */
    s_arc_label = lv_label_create(s_arc);
    lv_label_set_text(s_arc_label, "0%");
    lv_obj_center(s_arc_label);
    lv_obj_set_style_text_color(s_arc_label, lv_color_hex(0x4169E1), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_arc_label, &lv_font_montserrat_24, LV_PART_MAIN);
    
    /* 初始化圆弧值为0% */
    s_arc_value = 0;
    
    /* 创建定时器，每1秒更新一次 */
    s_arc_timer = lv_timer_create(arc_timer_cb, 1000, NULL);
    
    ESP_LOGI(TAG, "UI创建完成，加载圆圈开始更新");
}

/**
 * @brief 测试UI界面
 */
 void test_ui_create(void)
 {
     /* 设置屏幕背景色为深蓝色 */
     lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x001122), LV_PART_MAIN);
     
     /* 1. 显示标题文字 */
     lv_obj_t *title = lv_label_create(lv_scr_act());
     lv_label_set_text(title, "Hello LVGL!");
     lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
     lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN);
     lv_obj_align(title, LV_ALIGN_CENTER, 0, -60);
     
     /* 2. 显示第二行文字 */
     lv_obj_t *label2 = lv_label_create(lv_scr_act());
     lv_label_set_text(label2, "ESP32 + ST7789");
     lv_obj_set_style_text_color(label2, lv_color_hex(0x00FF00), LV_PART_MAIN);
     lv_obj_align(label2, LV_ALIGN_CENTER, 0, -30);
     
     /* 3. 显示第三行文字 */
     lv_obj_t *label3 = lv_label_create(lv_scr_act());
     lv_label_set_text(label3, "240x240 Display");
     lv_obj_set_style_text_color(label3, lv_color_hex(0x00FFFF), LV_PART_MAIN);
     lv_obj_align(label3, LV_ALIGN_CENTER, 0, 0);
     
     /* 4. 创建动画对象 - 一个圆形 */
     anim_obj = lv_obj_create(lv_scr_act());
     lv_obj_set_size(anim_obj, ANIM_OBJ_SIZE, ANIM_OBJ_SIZE);
     lv_obj_set_style_bg_color(anim_obj, lv_color_hex(0xFF00FF), LV_PART_MAIN);
     lv_obj_set_style_radius(anim_obj, LV_RADIUS_CIRCLE, LV_PART_MAIN);
     lv_obj_clear_flag(anim_obj, LV_OBJ_FLAG_SCROLLABLE);
     lv_obj_set_pos(anim_obj, ANIM_X_MIN, ANIM_Y_POS);
     lv_obj_move_foreground(anim_obj);  // 移到最上层，确保不被其他对象覆盖
     
     /* 5. 使用LVGL动画系统创建X轴移动动画 */
     lv_anim_t anim_x;
     lv_anim_init(&anim_x);
     lv_anim_set_var(&anim_x, anim_obj);
     lv_anim_set_values(&anim_x, ANIM_X_MIN, ANIM_X_MAX);
     lv_anim_set_time(&anim_x, ANIM_DURATION_MS);
     lv_anim_set_exec_cb(&anim_x, (lv_anim_exec_xcb_t)lv_obj_set_x);
     lv_anim_set_playback_time(&anim_x, ANIM_DURATION_MS);
     lv_anim_set_playback_delay(&anim_x, 0);
     lv_anim_set_repeat_delay(&anim_x, 0);
     lv_anim_set_repeat_count(&anim_x, LV_ANIM_REPEAT_INFINITE);
     lv_anim_set_path_cb(&anim_x, lv_anim_path_linear);
     lv_anim_start(&anim_x);
     
     /* 6. 显示彩色矩形 - 测试颜色显示 */
     /* 红色矩形 */
     lv_obj_t *rect1 = lv_obj_create(lv_scr_act());
     lv_obj_set_size(rect1, 50, 50);
     lv_obj_align(rect1, LV_ALIGN_CENTER, -60, 50);
     lv_obj_set_style_bg_color(rect1, lv_color_hex(0xFF0000), LV_PART_MAIN);
     lv_obj_set_style_radius(rect1, 5, LV_PART_MAIN);
     lv_obj_clear_flag(rect1, LV_OBJ_FLAG_SCROLLABLE);
     
     /* 绿色矩形 */
     lv_obj_t *rect2 = lv_obj_create(lv_scr_act());
     lv_obj_set_size(rect2, 50, 50);
     lv_obj_align(rect2, LV_ALIGN_CENTER, 0, 50);
     lv_obj_set_style_bg_color(rect2, lv_color_hex(0x00FF00), LV_PART_MAIN);
     lv_obj_set_style_radius(rect2, 5, LV_PART_MAIN);
     lv_obj_clear_flag(rect2, LV_OBJ_FLAG_SCROLLABLE);
     
     /* 蓝色矩形 */
     lv_obj_t *rect3 = lv_obj_create(lv_scr_act());
     lv_obj_set_size(rect3, 50, 50);
     lv_obj_align(rect3, LV_ALIGN_CENTER, 60, 50);
     lv_obj_set_style_bg_color(rect3, lv_color_hex(0x0000FF), LV_PART_MAIN);
     lv_obj_set_style_radius(rect3, 5, LV_PART_MAIN);
     lv_obj_clear_flag(rect3, LV_OBJ_FLAG_SCROLLABLE);
     
     ESP_LOGI(TAG, "测试UI界面创建完成，动画已启动");
 }
 
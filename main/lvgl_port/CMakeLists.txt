idf_component_register(
    SRCS "lvgl_ui_config.c" "lv_port_disp.c" "lvgl_task.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl st7789
)

idf_component_register(
    SRCS "mpu6050_task.c" "mpu6050.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_timer
)

target_link_libraries(${COMPONENT_LIB} m)
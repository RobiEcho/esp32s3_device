idf_component_register(
    SRCS "mpu6050.c"
    INCLUDE_DIRS "include"
    REQUIRES driver
)

# 链接数学库
target_link_libraries(${COMPONENT_LIB} m)
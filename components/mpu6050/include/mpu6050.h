#ifndef __MPU6050_DRIVER_H__
#define __MPU6050_DRIVER_H__

#include "driver/i2c_master.h"
#include "mpu6050_config.h"
#include "esp_err.h"
#include <stdbool.h>

typedef struct {
    int16_t accel_x;   // 加速度 X
    int16_t accel_y;   // 加速度 Y
    int16_t accel_z;   // 加速度 Z
    int16_t gyro_x;    // 陀螺仪 X
    int16_t gyro_y;    // 陀螺仪 Y
    int16_t gyro_z;    // 陀螺仪 Z
} mpu6050_data_t;

typedef struct {
    int16_t gyro_x_bias;
    int16_t gyro_y_bias;
    int16_t gyro_z_bias;
} mpu6050_gyro_bias_t;

esp_err_t mpu6050_init(void);
bool mpu6050_is_inited(void);

/**
 * @brief 读取陀螺仪六轴数据
 * @param data 存储读取结果的结构体指针
 */
esp_err_t mpu6050_read_data(mpu6050_data_t *data);

/**
 * @brief 校准陀螺仪零偏
 * @param bias 存储校准结果的结构体指针
 * @param samples 采样次数
 */
esp_err_t mpu6050_calibrate_gyro(mpu6050_gyro_bias_t *bias, uint16_t samples);

/**
 * @brief 计算角度变化（通过积分）
 * @param prev_angle 上一周期的角度值
 * @param gyro_rate 当前角速度（度/秒）
 * @param dt 时间间隔（秒）
 * @return 更新后的角度值
 */
float mpu6050_calculate_angle(float prev_angle, float gyro_rate, float dt);

#endif // __MPU6050_DRIVER_H__
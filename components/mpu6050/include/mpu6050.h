#ifndef __MPU6050_DRIVER_H__
#define __MPU6050_DRIVER_H__

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief MPU6050 原始传感器数据结构
 */
typedef struct {
    int16_t accel_x;   // 加速度 X 轴（原始值）
    int16_t accel_y;   // 加速度 Y 轴（原始值）
    int16_t accel_z;   // 加速度 Z 轴（原始值）
    int16_t gyro_x;    // 陀螺仪 X 轴（原始值）
    int16_t gyro_y;    // 陀螺仪 Y 轴（原始值）
    int16_t gyro_z;    // 陀螺仪 Z 轴（原始值）
} mpu6050_raw_data_t;

/**
 * @brief MPU6050 转换后的传感器数据结构（物理单位）
 */
typedef struct {
    float accel_x;     // 加速度 X 轴（m/s²）
    float accel_y;     // 加速度 Y 轴（m/s²）
    float accel_z;     // 加速度 Z 轴（m/s²）
    float gyro_x;      // 陀螺仪 X 轴（°/s）
    float gyro_y;      // 陀螺仪 Y 轴（°/s）
    float gyro_z;      // 陀螺仪 Z 轴（°/s）
} mpu6050_data_t;

/**
 * @brief 陀螺仪零偏校准数据结构
 */
typedef struct {
    int16_t gyro_x_bias;   // X 轴零偏
    int16_t gyro_y_bias;   // Y 轴零偏
    int16_t gyro_z_bias;   // Z 轴零偏
} mpu6050_gyro_bias_t;

/**
 * @brief 初始化 MPU6050 驱动
 * 
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t mpu6050_init(void);

/**
 * @brief 检查 MPU6050 是否已初始化
 * 
 * @return true 已初始化，false 未初始化
 */
bool mpu6050_is_inited(void);

/**
 * @brief 读取 MPU6050 原始数据（未转换）
 * 
 * @param data 存储读取结果的结构体指针
 * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 未初始化，ESP_ERR_INVALID_ARG 参数无效，其他值表示读取失败
 */
esp_err_t mpu6050_read_raw_data(mpu6050_raw_data_t *data);

/**
 * @brief 将原始数据转换为物理单位
 * 
 * @param raw_data 原始数据
 * @param data 转换后的数据
 * @return ESP_OK 成功，ESP_ERR_INVALID_ARG 参数无效
 */
esp_err_t mpu6050_convert_data(const mpu6050_raw_data_t *raw_data, mpu6050_data_t *data);

/**
 * @brief 读取 MPU6050 转换后的数据（物理单位）
 * 
 * @param data 存储读取结果的结构体指针
 * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 未初始化，ESP_ERR_INVALID_ARG 参数无效，其他值表示读取失败
 */
esp_err_t mpu6050_read_data(mpu6050_data_t *data);

/**
 * @brief 校准陀螺仪零偏
 * 
 * @param bias 存储校准结果的结构体指针
 * @param samples 采样次数（建议 100-200）
 * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 未初始化，ESP_ERR_INVALID_ARG 参数无效，其他值表示校准失败
 */
esp_err_t mpu6050_calibrate_gyro(mpu6050_gyro_bias_t *bias, uint16_t samples);

/**
 * @brief 设置陀螺仪量程
 * 
 * @param range 量程值（MPU6050_GYRO_RANGE_250/500/1000/2000）
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t mpu6050_set_gyro_range(uint8_t range);

/**
 * @brief 设置加速度计量程
 * 
 * @param range 量程值（MPU6050_ACCEL_RANGE_2G/4G/8G/16G）
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t mpu6050_set_accel_range(uint8_t range);

/**
 * @brief 获取陀螺仪灵敏度（LSB/°/s）
 * 
 * @return 灵敏度值
 */
float mpu6050_get_gyro_sensitivity(void);

/**
 * @brief 获取加速度计灵敏度（LSB/g）
 * 
 * @return 灵敏度值
 */
float mpu6050_get_accel_sensitivity(void);

/**
 * @brief 计算角度变化（通过积分）
 * 
 * @param prev_angle 上一周期的角度值（度）
 * @param gyro_rate 当前角速度（°/s）
 * @param dt 时间间隔（秒）
 * @return 更新后的角度值（度），范围 [0, 180]
 */
float mpu6050_calculate_angle(float prev_angle, float gyro_rate, float dt);

#endif // __MPU6050_DRIVER_H__

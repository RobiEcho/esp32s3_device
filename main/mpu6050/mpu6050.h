#ifndef __MPU6050_H__
#define __MPU6050_H__

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 陀螺仪原始数据结构体
 * @note 各轴数据为原始ADC值，需根据量程转换
 */
typedef struct {
    int16_t x;  // X轴角速度原始值
    int16_t y;  // Y轴角速度原始值
    int16_t z;  // Z轴角速度原始值
} mpu6050_data_t;

/**
 * @brief 初始化MPU6050传感器
 * @note 包含I2C总线初始化、设备注册和唤醒操作
 */
void mpu6050_init(void);

/**
 * @brief 读取陀螺仪三轴原始数据
 * @param data 存储读取结果的结构体指针
 */
void mpu6050_read_gyro(mpu6050_data_t *data);

/**
 * @brief 校准陀螺仪零偏
 * @param bias 存储校准结果的结构体指针
 * @param samples 采样次数（建议100次以上）
 */
void mpu6050_calibrate_gyro(mpu6050_data_t *bias, uint16_t samples);

/**
 * @brief 计算角度变化（通过积分）
 * @param prev_angle 上一周期的角度值
 * @param gyro_rate 当前角速度（度/秒）
 * @param dt 时间间隔（秒）
 * @return 更新后的角度值
 */
float mpu6050_calculate_angle(float prev_angle, float gyro_rate, float dt);

#ifdef __cplusplus
}
#endif

#endif //__MPU6050_H__
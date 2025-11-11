#ifndef __MPU6050_H__
#define __MPU6050_H__

#include "driver/i2c_master.h"

/* MPU6050配置 */
#define MPU6050_I2C_ADDR        0x68       // I2C_ADDR
#define MPU6050_I2C_PORT        I2C_NUM_0  // I2C_PORT
#define MPU6050_SDA_PIN         1          // I2C_SDA
#define MPU6050_SCL_PIN         0          // I2C_SCL
#define MPU6050_CLK_SPEED       400000     // I2C通信频率(400kHz)
#define GYRO_SCALE              131.0f     // ±250dps灵敏度

/**
 * @brief 陀螺仪原始数据结构体
 * @note 各轴数据为原始ADC值，需根据灵敏度转换
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
 * @param samples 采样次数
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

#endif //__MPU6050_H__
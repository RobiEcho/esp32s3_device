#ifndef __MPU6050_CONFIG_H__
#define __MPU6050_CONFIG_H__

/* ================= MPU6050 Hardware Config ================= */
#define MPU6050_I2C_ADDR        0x68          // I2C_ADDR
#define MPU6050_I2C_PORT        I2C_NUM_0     // I2C_PORT
#define MPU6050_SDA_PIN         (1)           // I2C_SDA
#define MPU6050_SCL_PIN         (0)           // I2C_SCL
#define MPU6050_I2C_CLK_SPEED   400000U       // I2C通信频率(400kHz)
#define MPU6050_I2C_GLITCH_IGNORE_CNT   (7)   // I2C误差忽略计数
#define MPU6050_GYRO_LSB_PER_DPS   131.0f     // 陀螺仪灵敏度(LSB/°/s)
#define MPU6050_DATA_BYTES(x)   ((x) * 2)     // 传感器数据字节数

/* ================= MPU6050 Register Addresses ============== */
#define MPU6050_REG_PWR_MGMT_1        0x6B    // 电源管理寄存器
#define MPU6050_REG_ACCEL_XOUT_H      0x3B    // 加速度计X轴高字节寄存器
#define MPU6050_REG_ACCEL_XOUT_L      0x3C    // 加速度计X轴低字节寄存器
#define MPU6050_REG_ACCEL_YOUT_H      0x3D    // 加速度计Y轴高字节寄存器
#define MPU6050_REG_ACCEL_YOUT_L      0x3E    // 加速度计Y轴低字节寄存器
#define MPU6050_REG_ACCEL_ZOUT_H      0x3F    // 加速度计Z轴高字节寄存器
#define MPU6050_REG_ACCEL_ZOUT_L      0x40    // 加速度计Z轴低字节寄存器
#define MPU6050_REG_GYRO_XOUT_H       0x43    // 陀螺仪X轴高字节寄存器
#define MPU6050_REG_GYRO_XOUT_L       0x44    // 陀螺仪X轴低字节寄存器
#define MPU6050_REG_GYRO_YOUT_H       0x45    // 陀螺仪Y轴低字节寄存器
#define MPU6050_REG_GYRO_YOUT_L       0x46    // 陀螺仪Y轴低字节寄存器
#define MPU6050_REG_GYRO_ZOUT_H       0x47    // 陀螺仪Z轴高字节寄存器
#define MPU6050_REG_GYRO_ZOUT_L       0x48    // 陀螺仪Z轴低字节寄存器

/* ================= PWR_MGMT_1 Register ===================== */
#define MPU6050_PWR_MGMT_1_SLEEP_BIT  (1 << 6)

#define MPU6050_PWR_MGMT_1_WAKEUP     0x00
#define MPU6050_PWR_MGMT_1_SLEEP      MPU6050_PWR_MGMT_1_SLEEP_BIT

#endif // __MPU6050_CONFIG_H__
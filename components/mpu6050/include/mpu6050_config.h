#ifndef __MPU6050_CONFIG_H__
#define __MPU6050_CONFIG_H__

/* ================= I2C Hardware Config ================= */
#define MPU6050_I2C_PORT                I2C_NUM_0       // I2C 端口号
#define MPU6050_SDA_PIN                 1               // SDA 引脚
#define MPU6050_SCL_PIN                 0               // SCL 引脚
#define MPU6050_I2C_ADDR                0x68            // I2C 从地址
#define MPU6050_I2C_CLK_SPEED           400000U         // I2C 时钟频率 (400kHz)
#define MPU6050_I2C_GLITCH_IGNORE_CNT   7               // 毛刺忽略计数

/* ================= Sensor Sensitivity ================= */
#define MPU6050_GYRO_LSB_PER_DPS        131.0f          // 陀螺仪灵敏度 (±250°/s)
#define MPU6050_ACCEL_LSB_PER_G         16384.0f        // 加速度计灵敏度 (±2g)
#define MPU6050_GRAVITY_MS2             9.80665f        // 重力加速度

/* ================= Gyro Sensitivity Table ================= */
#define MPU6050_GYRO_SENS_250           131.0f          // ±250°/s
#define MPU6050_GYRO_SENS_500           65.5f           // ±500°/s
#define MPU6050_GYRO_SENS_1000          32.8f           // ±1000°/s
#define MPU6050_GYRO_SENS_2000          16.4f           // ±2000°/s

/* ================= Accel Sensitivity Table ================= */
#define MPU6050_ACCEL_SENS_2G           16384.0f        // ±2g
#define MPU6050_ACCEL_SENS_4G           8192.0f         // ±4g
#define MPU6050_ACCEL_SENS_8G           4096.0f         // ±8g
#define MPU6050_ACCEL_SENS_16G          2048.0f         // ±16g

/* ================= Gyro Range Options ================= */
#define MPU6050_GYRO_RANGE_250          0x00            // ±250°/s
#define MPU6050_GYRO_RANGE_500          0x08            // ±500°/s
#define MPU6050_GYRO_RANGE_1000         0x10            // ±1000°/s
#define MPU6050_GYRO_RANGE_2000         0x18            // ±2000°/s

/* ================= Accel Range Options ================= */
#define MPU6050_ACCEL_RANGE_2G          0x00            // ±2g
#define MPU6050_ACCEL_RANGE_4G          0x08            // ±4g
#define MPU6050_ACCEL_RANGE_8G          0x10            // ±8g
#define MPU6050_ACCEL_RANGE_16G         0x18            // ±16g

/* ================= Range Values for Logging ================= */
#define MPU6050_GYRO_RANGE_250_VAL      250             // 250°/s
#define MPU6050_GYRO_RANGE_500_VAL      500             // 500°/s
#define MPU6050_GYRO_RANGE_1000_VAL     1000            // 1000°/s
#define MPU6050_GYRO_RANGE_2000_VAL     2000            // 2000°/s

#define MPU6050_ACCEL_RANGE_2G_VAL      2               // 2g
#define MPU6050_ACCEL_RANGE_4G_VAL      4               // 4g
#define MPU6050_ACCEL_RANGE_8G_VAL      8               // 8g
#define MPU6050_ACCEL_RANGE_16G_VAL     16              // 16g

/* ================= Default Range ================= */
#define MPU6050_DEFAULT_GYRO_RANGE      MPU6050_GYRO_RANGE_250
#define MPU6050_DEFAULT_ACCEL_RANGE     MPU6050_ACCEL_RANGE_2G

/* ================= Register Addresses ================= */
#define MPU6050_REG_PWR_MGMT_1          0x6B            // 电源管理寄存器
#define MPU6050_REG_PWR_MGMT_2          0x6C            // 电源管理寄存器 2
#define MPU6050_REG_CONFIG              0x1A            // 配置寄存器
#define MPU6050_REG_GYRO_CONFIG         0x1B            // 陀螺仪配置寄存器
#define MPU6050_REG_ACCEL_CONFIG        0x1C            // 加速度计配置寄存器
#define MPU6050_REG_ACCEL_XOUT_H        0x3B            // 加速度 X 高字节
#define MPU6050_REG_ACCEL_XOUT_L        0x3C            // 加速度 X 低字节
#define MPU6050_REG_ACCEL_YOUT_H        0x3D            // 加速度 Y 高字节
#define MPU6050_REG_ACCEL_YOUT_L        0x3E            // 加速度 Y 低字节
#define MPU6050_REG_ACCEL_ZOUT_H        0x3F            // 加速度 Z 高字节
#define MPU6050_REG_ACCEL_ZOUT_L        0x40            // 加速度 Z 低字节
#define MPU6050_REG_TEMP_OUT_H          0x41            // 温度高字节
#define MPU6050_REG_TEMP_OUT_L          0x42            // 温度低字节
#define MPU6050_REG_GYRO_XOUT_H         0x43            // 陀螺仪 X 高字节
#define MPU6050_REG_GYRO_XOUT_L         0x44            // 陀螺仪 X 低字节
#define MPU6050_REG_GYRO_YOUT_H         0x45            // 陀螺仪 Y 高字节
#define MPU6050_REG_GYRO_YOUT_L         0x46            // 陀螺仪 Y 低字节
#define MPU6050_REG_GYRO_ZOUT_H         0x47            // 陀螺仪 Z 高字节
#define MPU6050_REG_GYRO_ZOUT_L         0x48            // 陀螺仪 Z 低字节
#define MPU6050_REG_USER_CTRL           0x6A            // 用户控制寄存器
#define MPU6050_REG_WHO_AM_I            0x75            // 设备 ID 寄存器

/* ================= Register Bits ================= */
#define MPU6050_PWR_MGMT_1_SLEEP_BIT    (1 << 6)        // 睡眠位
#define MPU6050_PWR_MGMT_1_DEVICE_RESET (1 << 7)        // 设备重置位
#define MPU6050_PWR_MGMT_1_WAKEUP       0x00            // 唤醒值
#define MPU6050_PWR_MGMT_1_SLEEP        (1 << 6)        // 睡眠值
#define MPU6050_WHO_AM_I_VALUE          0x68            // 设备 ID 预期值

/* ================= Data Format ================= */
#define MPU6050_DATA_BYTES(x)           ((x) * 2)       // 数据字节数计算
#define MPU6050_FULL_DATA_BYTES         14              // 完整数据字节数 (加速度6 + 温度2 + 陀螺仪6)

#endif // __MPU6050_CONFIG_H__

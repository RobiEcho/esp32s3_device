#include "mpu6050.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <arpa/inet.h>
#include <math.h>

static i2c_master_bus_handle_t bus_handle = NULL; // I2C总线句柄
static i2c_master_dev_handle_t dev_handle = NULL; // I2C设备句柄

static const char *TAG = "MPU6050";

static esp_err_t i2c_bus_init(void) 
{
    // I2C总线配置
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = MPU6050_I2C_PORT,           //  I2C端口
        .sda_io_num = MPU6050_SDA_PIN,          //  SDA引脚
        .scl_io_num = MPU6050_SCL_PIN,          //  SCL引脚
        .clk_source = I2C_CLK_SRC_DEFAULT,      //  时钟源
        .glitch_ignore_cnt = 7,                 //  误差忽略计数
        .flags.enable_internal_pullup = true    // 启用内部上拉电阻
    };
    
    return i2c_new_master_bus(&bus_cfg, &bus_handle); //  创建I2C总线
}

void mpu6050_init(void) 
{
    // 初始化I2C总线
    ESP_ERROR_CHECK(i2c_bus_init());
    
    // 配置MPU6050设备
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_I2C_ADDR,
        .scl_speed_hz = MPU6050_CLK_SPEED,
    };
    
    // 添加主设备到总线
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    // 唤醒设备：写PWR_MGMT_1寄存器（0x6B），设置值为0x00
    uint8_t wakeup_cmd[] = {0x6B, 0x00};
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, wakeup_cmd, sizeof(wakeup_cmd), -1));
    
    ESP_LOGI(TAG, "MPU6050 初始化完成");
}

void mpu6050_read_gyro(mpu6050_data_t *data) 
{
    uint8_t reg_addr = 0x43;  // 陀螺仪测量值读取地址
    uint8_t raw_data[6];      // 存储6字节原始数据（X/Y/Z各2字节）

    // 读数据
    ESP_ERROR_CHECK(i2c_master_transmit_receive(
        dev_handle,
        &reg_addr, 1,               // 发送寄存器地址与地址大小（字节）
        raw_data, sizeof(raw_data), // 接收数据
        100                         // 100ms超时时间
    ));
    uint16_t x_be, y_be, z_be;

    memcpy(&x_be, &raw_data[0], 2);
    memcpy(&y_be, &raw_data[2], 2);
    memcpy(&z_be, &raw_data[4], 2);

    data->x = (int16_t)ntohs(x_be);
    data->y = (int16_t)ntohs(y_be);
    data->z = (int16_t)ntohs(z_be);
}

void mpu6050_calibrate_gyro(mpu6050_data_t *bias, uint16_t samples) 
{
    mpu6050_data_t temp;
    bias->x = 0;
    bias->y = 0;
    bias->z = 0;
    
    // 采集指定次数的样本
    for (uint16_t i = 0; i < samples; i++) {
        mpu6050_read_gyro(&temp);
        bias->x += temp.x;
        bias->y += temp.y;
        bias->z += temp.z;
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms间隔
    }

    bias->x /= samples;
    bias->y /= samples;
    bias->z /= samples;
    
    ESP_LOGI(TAG, "校准陀螺仪零偏完成");
}

float mpu6050_calculate_angle(float prev_angle, float gyro_rate, float dt) 
{
    float new_angle = prev_angle + ((gyro_rate / GYRO_SCALE) * dt);
    
    // 防抖动
    if (fabsf(new_angle - prev_angle) < 2) {
        return prev_angle;
    }
    
    // 输出的结果限定在0~180，防止角度越界
    if (new_angle < 0) {
        return 0;
    } else if (new_angle > 180) {
        return 180;
    }
    
    return new_angle;
}
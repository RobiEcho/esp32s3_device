#include "mpu6050.h"
#include "freertos/FreeRTOS.h"
#include <math.h>

#define NUM_SENSOR_DATA 6  // 加速度计和陀螺仪各3轴数据
static i2c_master_bus_handle_t s_bus_handle = NULL; 
static i2c_master_dev_handle_t s_dev_handle = NULL;
static bool s_inited = false;

static esp_err_t _i2c_bus_init(void) 
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = MPU6050_I2C_PORT,                       // I2C0
        .sda_io_num = MPU6050_SDA_PIN,                      // SDA引脚
        .scl_io_num = MPU6050_SCL_PIN,                      // SCL引脚
        .clk_source = I2C_CLK_SRC_DEFAULT,                  // 默认时钟
        .glitch_ignore_cnt = MPU6050_I2C_GLITCH_IGNORE_CNT, // 误差忽略计数
        .flags.enable_internal_pullup = true                // 上拉
    };
    
    return i2c_new_master_bus(&bus_cfg, &s_bus_handle);
}

esp_err_t mpu6050_init(void) 
{
    if (s_inited) {
        return ESP_OK;
    }

    // 初始化I2C总线
    esp_err_t ret = _i2c_bus_init();
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 配置MPU6050设备
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_I2C_ADDR,
        .scl_speed_hz = MPU6050_I2C_CLK_SPEED,
    };
    
    // 添加设备到总线
    ret = i2c_master_bus_add_device(s_bus_handle, &dev_cfg, &s_dev_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    uint8_t wakeup_cmd[] = { MPU6050_REG_PWR_MGMT_1, MPU6050_PWR_MGMT_1_WAKEUP };
    ret = i2c_master_transmit(s_dev_handle, wakeup_cmd, sizeof(wakeup_cmd), -1);
    if (ret != ESP_OK) {
        return ret;
    }

    s_inited = true;
    return ESP_OK;
}

bool mpu6050_is_inited(void)
{
    return s_inited;
}

esp_err_t mpu6050_read_data(mpu6050_data_t *data)
{
    if (!s_inited || s_dev_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t reg_addr = MPU6050_REG_ACCEL_XOUT_H;
    uint8_t raw_data[MPU6050_DATA_BYTES(NUM_SENSOR_DATA)];

    // 读数据
    esp_err_t ret = i2c_master_transmit_receive(
        s_dev_handle,
        &reg_addr, 1,               // 发送寄存器地址与地址大小（字节）
        raw_data, sizeof(raw_data), // 接收数据
        100                         // 100ms超时时间
    );
    if (ret != ESP_OK) {
        return ret;
    }

    data->accel_x = (int16_t)((raw_data[0] << 8) | raw_data[1]);
    data->accel_y = (int16_t)((raw_data[2] << 8) | raw_data[3]);
    data->accel_z = (int16_t)((raw_data[4] << 8) | raw_data[5]);
    data->gyro_x  = (int16_t)((raw_data[6] << 8) | raw_data[7]);
    data->gyro_y  = (int16_t)((raw_data[8] << 8) | raw_data[9]);
    data->gyro_z  = (int16_t)((raw_data[10] << 8) | raw_data[11]);

    return ESP_OK;
}

esp_err_t mpu6050_calibrate_gyro(mpu6050_gyro_bias_t *bias, uint16_t samples)
{
    if (!s_inited) {
        return ESP_ERR_INVALID_STATE;
    }
    if (bias == NULL || samples == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    mpu6050_data_t temp;
    int32_t sum_x = 0;
    int32_t sum_y = 0;
    int32_t sum_z = 0;

    for (uint16_t i = 0; i < samples; i++) {
        esp_err_t ret = mpu6050_read_data(&temp);
        if (ret != ESP_OK) {
            return ret;
        }

        sum_x += temp.gyro_x;
        sum_y += temp.gyro_y;
        sum_z += temp.gyro_z;

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    bias->gyro_x_bias = sum_x / samples;
    bias->gyro_y_bias = sum_y / samples;
    bias->gyro_z_bias = sum_z / samples;

    return ESP_OK;
}

float mpu6050_calculate_angle(float prev_angle, float gyro_rate, float dt) 
{
    float new_angle = prev_angle + ((gyro_rate / MPU6050_GYRO_LSB_PER_DPS) * dt);
    
    // 防抖动
    if (fabsf(new_angle - prev_angle) < 2) {
        return prev_angle;
    }
    
    if (new_angle < 0) {
        return 0;
    } else if (new_angle > 180) {
        return 180;
    }
    
    return new_angle;
}
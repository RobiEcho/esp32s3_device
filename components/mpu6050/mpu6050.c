#include "mpu6050.h"
#include "mpu6050_config.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "mpu6050";

#define NUM_SENSOR_DATA 7  // 加速度计3轴 + 温度 + 陀螺仪3轴

static i2c_master_bus_handle_t s_bus_handle = NULL;
static i2c_master_dev_handle_t s_dev_handle = NULL;
static bool s_inited = false;

// 灵敏度表
static float s_gyro_sensitivity = MPU6050_GYRO_SENS_250;       // 默认 ±250°/s
static float s_accel_sensitivity = MPU6050_ACCEL_SENS_2G;      // 默认 ±2g

// 陀螺仪零偏
static mpu6050_gyro_bias_t s_gyro_bias = {0, 0, 0};
static bool s_gyro_calibrated = false;

static esp_err_t _i2c_bus_init(void)
{
    if (s_bus_handle != NULL) {
        ESP_LOGW(TAG, "I2C 总线已初始化");
        return ESP_OK;
    }

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = MPU6050_I2C_PORT,
        .sda_io_num = MPU6050_SDA_PIN,
        .scl_io_num = MPU6050_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = MPU6050_I2C_GLITCH_IGNORE_CNT,
        .flags.enable_internal_pullup = true
    };

    esp_err_t err = i2c_new_master_bus(&bus_cfg, &s_bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "初始化 I2C 总线失败: %s", esp_err_to_name(err));
        s_bus_handle = NULL;
        return err;
    }

    return ESP_OK;
}

static esp_err_t _mpu6050_device_init(void)
{
    if (s_dev_handle != NULL) {
        ESP_LOGW(TAG, "MPU6050 设备已初始化");
        return ESP_OK;
    }

    if (s_bus_handle == NULL) {
        ESP_LOGE(TAG, "I2C 总线未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_I2C_ADDR,
        .scl_speed_hz = MPU6050_I2C_CLK_SPEED,
    };

    esp_err_t err = i2c_master_bus_add_device(s_bus_handle, &dev_cfg, &s_dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "添加 MPU6050 设备失败: %s", esp_err_to_name(err));
        s_dev_handle = NULL;
        return err;
    }

    return ESP_OK;
}

static esp_err_t _mpu6050_verify_device(void)
{
    if (s_dev_handle == NULL) {
        ESP_LOGE(TAG, "MPU6050 设备未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t who_am_i = 0;
    uint8_t reg_addr = MPU6050_REG_WHO_AM_I;

    esp_err_t err = i2c_master_transmit_receive(
        s_dev_handle,
        &reg_addr, 1,
        &who_am_i, 1,
        100  // 100ms 超时
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "读取 WHO_AM_I 失败: %s", esp_err_to_name(err));
        return err;
    }

    if (who_am_i != MPU6050_WHO_AM_I_VALUE) {
        ESP_LOGE(TAG, "设备 ID 不匹配，期望: 0x%02X，实际: 0x%02X",
                 MPU6050_WHO_AM_I_VALUE, who_am_i);
        return ESP_ERR_INVALID_RESPONSE;
    }

    ESP_LOGI(TAG, "设备验证成功，ID: 0x%02X", who_am_i);
    return ESP_OK;
}

static esp_err_t _mpu6050_wakeup(void)
{
    if (s_dev_handle == NULL) {
        ESP_LOGE(TAG, "MPU6050 设备未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t wakeup_cmd[] = {MPU6050_REG_PWR_MGMT_1, MPU6050_PWR_MGMT_1_WAKEUP};
    esp_err_t err = i2c_master_transmit(s_dev_handle, wakeup_cmd, sizeof(wakeup_cmd), -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "唤醒 MPU6050 失败: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t mpu6050_init(void)
{
    if (s_inited) {
        ESP_LOGW(TAG, "MPU6050 已初始化");
        return ESP_OK;
    }

    // 初始化 I2C 总线
    esp_err_t err = _i2c_bus_init();
    if (err != ESP_OK) {
        return err;
    }

    // 初始化 MPU6050 设备
    err = _mpu6050_device_init();
    if (err != ESP_OK) {
        return err;
    }

    // 验证设备是否正确连接
    err = _mpu6050_verify_device();
    if (err != ESP_OK) {
        return err;
    }

    // 唤醒 MPU6050
    err = _mpu6050_wakeup();
    if (err != ESP_OK) {
        return err;
    }

    s_inited = true;
    ESP_LOGI(TAG, "MPU6050 初始化成功");
    return ESP_OK;
}

bool mpu6050_is_inited(void)
{
    return s_inited;
}

esp_err_t mpu6050_read_raw_data(mpu6050_raw_data_t *data)
{
    if (!s_inited) {
        ESP_LOGE(TAG, "MPU6050 未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    if (data == NULL) {
        ESP_LOGE(TAG, "数据指针为空");
        return ESP_ERR_INVALID_ARG;
    }

    if (s_dev_handle == NULL) {
        ESP_LOGE(TAG, "设备句柄无效");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t reg_addr = MPU6050_REG_ACCEL_XOUT_H;
    uint8_t raw_data[MPU6050_DATA_BYTES(NUM_SENSOR_DATA)];

    esp_err_t err = i2c_master_transmit_receive(
        s_dev_handle,
        &reg_addr, 1,
        raw_data, sizeof(raw_data),
        100  // 100ms 超时
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "读取数据失败: %s", esp_err_to_name(err));
        return err;
    }

    // 解析原始数据（注意：加速度和陀螺仪之间有2字节温度数据）
    data->accel_x = (int16_t)((raw_data[0] << 8) | raw_data[1]);
    data->accel_y = (int16_t)((raw_data[2] << 8) | raw_data[3]);
    data->accel_z = (int16_t)((raw_data[4] << 8) | raw_data[5]);
    // raw_data[6], raw_data[7] 是温度数据，跳过
    data->gyro_x = (int16_t)((raw_data[8] << 8) | raw_data[9]);
    data->gyro_y = (int16_t)((raw_data[10] << 8) | raw_data[11]);
    data->gyro_z = (int16_t)((raw_data[12] << 8) | raw_data[13]);

    return ESP_OK;
}

esp_err_t mpu6050_convert_data(const mpu6050_raw_data_t *raw_data, mpu6050_data_t *data)
{
    if (raw_data == NULL || data == NULL) {
        ESP_LOGE(TAG, "数据指针为空");
        return ESP_ERR_INVALID_ARG;
    }

    // 转换加速度计数据（原始值 → m/s²）
    float accel_x_g = raw_data->accel_x / s_accel_sensitivity;
    float accel_y_g = raw_data->accel_y / s_accel_sensitivity;
    float accel_z_g = raw_data->accel_z / s_accel_sensitivity;

    data->accel_x = accel_x_g * MPU6050_GRAVITY_MS2;
    data->accel_y = accel_y_g * MPU6050_GRAVITY_MS2;
    data->accel_z = accel_z_g * MPU6050_GRAVITY_MS2;

    // 转换陀螺仪数据（原始值 - 零偏 → °/s）
    int16_t gyro_x_corrected = raw_data->gyro_x;
    int16_t gyro_y_corrected = raw_data->gyro_y;
    int16_t gyro_z_corrected = raw_data->gyro_z;

    if (s_gyro_calibrated) {
        gyro_x_corrected -= s_gyro_bias.gyro_x_bias;
        gyro_y_corrected -= s_gyro_bias.gyro_y_bias;
        gyro_z_corrected -= s_gyro_bias.gyro_z_bias;
    }

    data->gyro_x = gyro_x_corrected / s_gyro_sensitivity;
    data->gyro_y = gyro_y_corrected / s_gyro_sensitivity;
    data->gyro_z = gyro_z_corrected / s_gyro_sensitivity;

    return ESP_OK;
}

esp_err_t mpu6050_read_data(mpu6050_data_t *data)
{
    if (data == NULL) {
        ESP_LOGE(TAG, "数据指针为空");
        return ESP_ERR_INVALID_ARG;
    }

    mpu6050_raw_data_t raw_data;

    // 读取原始数据
    esp_err_t err = mpu6050_read_raw_data(&raw_data);
    if (err != ESP_OK) {
        return err;
    }

    // 转换为物理单位
    return mpu6050_convert_data(&raw_data, data);
}

esp_err_t mpu6050_calibrate_gyro(mpu6050_gyro_bias_t *bias, uint16_t samples)
{
    if (!s_inited) {
        ESP_LOGE(TAG, "MPU6050 未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    if (samples == 0) {
        ESP_LOGE(TAG, "采样次数不能为 0");
        return ESP_ERR_INVALID_ARG;
    }

    mpu6050_raw_data_t temp;
    int32_t sum_x = 0;
    int32_t sum_y = 0;
    int32_t sum_z = 0;

    ESP_LOGI(TAG, "开始校准陀螺仪，采样次数: %u", samples);

    for (uint16_t i = 0; i < samples; i++) {
        esp_err_t err = mpu6050_read_raw_data(&temp);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "校准过程中读取数据失败: %s", esp_err_to_name(err));
            return err;
        }

        sum_x += temp.gyro_x;
        sum_y += temp.gyro_y;
        sum_z += temp.gyro_z;

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    s_gyro_bias.gyro_x_bias = sum_x / samples;
    s_gyro_bias.gyro_y_bias = sum_y / samples;
    s_gyro_bias.gyro_z_bias = sum_z / samples;
    s_gyro_calibrated = true;

    ESP_LOGI(TAG, "陀螺仪校准完成 - X: %d, Y: %d, Z: %d",
             s_gyro_bias.gyro_x_bias, s_gyro_bias.gyro_y_bias, s_gyro_bias.gyro_z_bias);

    if (bias != NULL) {
        bias->gyro_x_bias = s_gyro_bias.gyro_x_bias;
        bias->gyro_y_bias = s_gyro_bias.gyro_y_bias;
        bias->gyro_z_bias = s_gyro_bias.gyro_z_bias;
    }

    return ESP_OK;
}

esp_err_t mpu6050_set_gyro_range(uint8_t range)
{
    if (!s_inited) {
        ESP_LOGE(TAG, "MPU6050 未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    if (s_dev_handle == NULL) {
        ESP_LOGE(TAG, "设备句柄无效");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t config[] = {MPU6050_REG_GYRO_CONFIG, range};
    esp_err_t err = i2c_master_transmit(s_dev_handle, config, sizeof(config), -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "设置陀螺仪量程失败: %s", esp_err_to_name(err));
        return err;
    }

    // 更新灵敏度
    switch (range) {
        case MPU6050_GYRO_RANGE_250:
            s_gyro_sensitivity = MPU6050_GYRO_SENS_250;
            ESP_LOGI(TAG, "陀螺仪量程已设置为 ±%d°/s", MPU6050_GYRO_RANGE_250_VAL);
            break;
        case MPU6050_GYRO_RANGE_500:
            s_gyro_sensitivity = MPU6050_GYRO_SENS_500;
            ESP_LOGI(TAG, "陀螺仪量程已设置为 ±%d°/s", MPU6050_GYRO_RANGE_500_VAL);
            break;
        case MPU6050_GYRO_RANGE_1000:
            s_gyro_sensitivity = MPU6050_GYRO_SENS_1000;
            ESP_LOGI(TAG, "陀螺仪量程已设置为 ±%d°/s", MPU6050_GYRO_RANGE_1000_VAL);
            break;
        case MPU6050_GYRO_RANGE_2000:
            s_gyro_sensitivity = MPU6050_GYRO_SENS_2000;
            ESP_LOGI(TAG, "陀螺仪量程已设置为 ±%d°/s", MPU6050_GYRO_RANGE_2000_VAL);
            break;
        default:
            ESP_LOGW(TAG, "陀螺仪量程设置值错误");
            return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

esp_err_t mpu6050_set_accel_range(uint8_t range)
{
    if (!s_inited) {
        ESP_LOGE(TAG, "MPU6050 未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    if (s_dev_handle == NULL) {
        ESP_LOGE(TAG, "设备句柄无效");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t config[] = {MPU6050_REG_ACCEL_CONFIG, range};
    esp_err_t err = i2c_master_transmit(s_dev_handle, config, sizeof(config), -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "设置加速度计量程失败: %s", esp_err_to_name(err));
        return err;
    }

    // 更新灵敏度
    switch (range) {
        case MPU6050_ACCEL_RANGE_2G:
            s_accel_sensitivity = MPU6050_ACCEL_SENS_2G;
            ESP_LOGI(TAG, "加速度计量程已设置为 ±%dg", MPU6050_ACCEL_RANGE_2G_VAL);
            break;
        case MPU6050_ACCEL_RANGE_4G:
            s_accel_sensitivity = MPU6050_ACCEL_SENS_4G;
            ESP_LOGI(TAG, "加速度计量程已设置为 ±%dg", MPU6050_ACCEL_RANGE_4G_VAL);
            break;
        case MPU6050_ACCEL_RANGE_8G:
            s_accel_sensitivity = MPU6050_ACCEL_SENS_8G;
            ESP_LOGI(TAG, "加速度计量程已设置为 ±%dg", MPU6050_ACCEL_RANGE_8G_VAL);
            break;
        case MPU6050_ACCEL_RANGE_16G:
            s_accel_sensitivity = MPU6050_ACCEL_SENS_16G;
            ESP_LOGI(TAG, "加速度计量程已设置为 ±%dg", MPU6050_ACCEL_RANGE_16G_VAL);
            break;
        default:
            ESP_LOGW(TAG, "加速度设计值错误");
            return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

float mpu6050_calculate_angle(float prev_angle, float gyro_rate, float dt)
{
    if (dt <= 0.0f) {
        ESP_LOGW(TAG, "时间间隔无效: %f", dt);
        return prev_angle;
    }

    float new_angle = prev_angle + (gyro_rate * dt);

    // 防抖动：角度变化小于 2° 时保持不变
    if (fabsf(new_angle - prev_angle) < 2.0f) {
        return prev_angle;
    }

    // 限制角度范围 [0, 180]
    if (new_angle < 0.0f) {
        return 0.0f;
    } else if (new_angle > 180.0f) {
        return 180.0f;
    }

    return new_angle;
}

float mpu6050_get_gyro_sensitivity(void)
{
    return s_gyro_sensitivity;
}

float mpu6050_get_accel_sensitivity(void)
{
    return s_accel_sensitivity;
}
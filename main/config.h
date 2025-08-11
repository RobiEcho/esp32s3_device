#ifndef __CONFIG_H__
#define __CONFIG_H__

/* WiFi配置 */
#define WIFI_SSID      "USER_E36498"
#define WIFI_PASS      "XX687722@"

/* MQTT配置 */
#define MQTT_URI       "mqtt://192.168.1.95:1883"
#define MQTT_CLIENT_ID "esp32s3_Client"
#define MQTT_RX_BUFFER_SIZE      8192 // 接收缓冲区大小
#define MQTT_RECONNECT_INTERVAL  10 // 重连间隔（秒）
#define MQTT_TOPIC       "6050_date"       // 发送6050数据主题
#define IMG_TOPIC        "6818_image"
#define IMG_WIDTH        240
#define IMG_HEIGHT       240
#define IMG_PIXEL_SIZE   2

/* MPU6050配置 */
#define MPU6050_I2C_ADDR        0x68    // 设备I2C地址（AD0引脚接地时的地址）
#define MPU6050_I2C_PORT        I2C_NUM_0  // 使用的I2C端口
#define MPU6050_SDA_PIN         1       // I2C数据线引脚
#define MPU6050_SCL_PIN         0       // I2C时钟线引脚
#define MPU6050_CLK_SPEED_HZ    100000  // I2C通信频率(100kHz)
#define GYRO_SCALE              131.0f  // ±250dps灵敏度

/* ST7789配置 */
#define ST7789_SPI_HOST       SPI3_HOST
#define ST7789_MOSI_PIN       11      // SPI数据线
#define ST7789_SCLK_PIN       12      // SPI时钟线
#define ST7789_RES_PIN        6       // 复位引脚
#define ST7789_DC_PIN         7       // 数据/命令控制引脚
#define ST7789_PIXEL_FORMAT   0x55    // 16-bit RGB565
#define ST7789_MAX_TRANS_SIZE 4096    // SPI单次传输最大字节数
#define ST7789_WIDTH          240
#define ST7789_HEIGHT         240

/* 应用配置 */
#define SAMPLING_MS    50

#endif /* __CONFIG_H__ */
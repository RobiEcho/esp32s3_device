#ifndef __MYMQTT_CONFIG_H__
#define __MYMQTT_CONFIG_H__

/* ================= Broker Config ================= */
#define MYMQTT_BROKER_URI          "mqtt://192.168.5.46:1883"
#define MYMQTT_CLIENT_ID           "esp32s3_client"
#define MYMQTT_USERNAME            "RobiEcho"
#define MYMQTT_PASSWORD            "123456"

/* ================= Buffer Config ================= */
#define MYMQTT_RX_BUFFER_SIZE      (16 * 1024)    // 接收缓冲区大小（16KB）

/* ================= Topic Config ================= */
#define MYMQTT_TOPIC_MPU6050       "esp32/mpu6050_data"   // MPU6050 数据主题
#define MYMQTT_TOPIC_IMAGE         "esp32/image"     // 接收图像主题

/* ================= Image Config ================= */
#define MYMQTT_IMG_WIDTH           240
#define MYMQTT_IMG_HEIGHT          240
#define MYMQTT_IMG_PIXEL_SIZE      2              // RGB565: 2字节/像素
#define MYMQTT_IMG_BUF_SIZE        (MYMQTT_IMG_WIDTH * MYMQTT_IMG_HEIGHT * MYMQTT_IMG_PIXEL_SIZE)
#define MYMQTT_IMG_TIMEOUT_US      (2000 * 1000)  // 图像接收超时（2秒）

#endif /* __MYMQTT_CONFIG_H__ */

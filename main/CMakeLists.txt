idf_component_register(
    SRCS "app_main.c"
    PRIV_REQUIRES spi_flash esp_timer wifi_config mpu6050 st7789 mymqtt
    INCLUDE_DIRS ""
)

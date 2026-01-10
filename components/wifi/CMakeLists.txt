idf_component_register(
    SRCS "wifi_credentials.c" "wifi.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_storage esp_wifi esp_event esp_netif
)
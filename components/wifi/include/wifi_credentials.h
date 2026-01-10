#ifndef __WIFI_CREDENTIALS_H__
#define __WIFI_CREDENTIALS_H__

#include "esp_err.h"

/**
 * @brief 保存 STA 模式的 WiFi 凭证到 NVS
 * 
 * @param ssid WiFi SSID
 * @param password WiFi 密码
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t wifi_credentials_save_sta(const char *ssid, const char *password);

/**
 * @brief 从 NVS 读取 STA 模式的 WiFi 凭证
 * 
 * @param ssid 输出缓冲区
 * @param ssid_len 缓冲区大小
 * @param password 输出缓冲区
 * @param pass_len 缓冲区大小
 * @return ESP_OK 成功，ESP_ERR_NVS_NOT_FOUND 凭证不存在，其他值表示失败
 */
esp_err_t wifi_credentials_load_sta(char *ssid, size_t ssid_len, char *password, size_t pass_len);

/**
 * @brief 保存 AP 模式的 WiFi 凭证到 NVS
 * 
 * @param ssid WiFi SSID
 * @param password WiFi 密码
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t wifi_credentials_save_ap(const char *ssid, const char *password);

/**
 * @brief 从 NVS 读取 AP 模式的 WiFi 凭证
 * 
 * @param ssid 输出缓冲区
 * @param ssid_len 缓冲区大小
 * @param password 输出缓冲区
 * @param pass_len 缓冲区大小
 * @return ESP_OK 成功，ESP_ERR_NVS_NOT_FOUND 凭证不存在，其他值表示失败
 */
esp_err_t wifi_credentials_load_ap(char *ssid, size_t ssid_len, char *password, size_t pass_len);

/**
 * @brief 删除 STA 凭证
 * 
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t wifi_credentials_erase_sta(void);

/**
 * @brief 删除 AP 凭证
 * 
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t wifi_credentials_erase_ap(void);

#endif /* __WIFI_CREDENTIALS_H__ */

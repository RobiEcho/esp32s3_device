#ifndef __NVS_STORAGE_H__
#define __NVS_STORAGE_H__

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief NVS 初始化状态标志（全局可访问）
 * 其他模块可以通过检查此标志来确认 NVS 是否已初始化
 */
extern bool g_nvs_initialized;

/**
 * @brief 初始化 NVS 存储
 * 
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t nvs_storage_init(void);

/**
 * @brief 保存字符串到 NVS
 * 
 * @param namespace_name 命名空间名称
 * @param key 键名
 * @param value 字符串值
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t nvs_storage_set_str(const char *namespace_name, const char *key, const char *value);

/**
 * @brief 从 NVS 读取字符串
 * 
 * @param namespace_name 命名空间名称
 * @param key 键名
 * @param value 输出缓冲区
 * @param value_len 缓冲区大小
 * @return ESP_OK 成功，ESP_ERR_NVS_NOT_FOUND 键不存在，其他值表示失败
 */
esp_err_t nvs_storage_get_str(const char *namespace_name, const char *key, char *value, size_t value_len);

/**
 * @brief 保存整数到 NVS
 * 
 * @param namespace_name 命名空间名称
 * @param key 键名
 * @param value 整数值
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t nvs_storage_set_i32(const char *namespace_name, const char *key, int32_t value);

/**
 * @brief 从 NVS 读取整数
 * 
 * @param namespace_name 命名空间名称
 * @param key 键名
 * @param value 输出指针
 * @return ESP_OK 成功，ESP_ERR_NVS_NOT_FOUND 键不存在，其他值表示失败
 */
esp_err_t nvs_storage_get_i32(const char *namespace_name, const char *key, int32_t *value);

/**
 * @brief 删除 NVS 中的键
 * 
 * @param namespace_name 命名空间名称
 * @param key 键名
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t nvs_storage_erase_key(const char *namespace_name, const char *key);

/**
 * @brief 删除整个命名空间
 * 
 * @param namespace_name 命名空间名称
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t nvs_storage_erase_namespace(const char *namespace_name);

#endif /* __NVS_STORAGE_H__ */

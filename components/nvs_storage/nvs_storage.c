#include "nvs_storage.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "nvs_storage";

static bool s_inited = false;

// 全局初始化状态标志，其他模块可以通过 extern 访问
bool g_nvs_initialized = false;

esp_err_t nvs_storage_init(void)
{
    if (s_inited) {
        return ESP_OK;
    }

    // 初始化 NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS 分区需要擦除，正在擦除...");
        err = nvs_flash_erase();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "擦除 NVS 分区失败: %s", esp_err_to_name(err));
            return err;
        }
        err = nvs_flash_init();
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "初始化 NVS 失败: %s", esp_err_to_name(err));
        return err;
    }

    s_inited = true;
    g_nvs_initialized = true;
    return ESP_OK;
}

esp_err_t nvs_storage_set_str(const char *namespace_name, const char *key, const char *value)
{
    if (!s_inited) {
        ESP_LOGE(TAG, "NVS 未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    if (namespace_name == NULL || key == NULL || value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "打开命名空间 '%s' 失败: %s", namespace_name, esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(nvs_handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "设置键 '%s' 失败: %s", key, esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "提交更改失败: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}

esp_err_t nvs_storage_get_str(const char *namespace_name, const char *key, char *value, size_t value_len)
{
    if (!s_inited) {
        ESP_LOGE(TAG, "NVS 未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    if (namespace_name == NULL || key == NULL || value == NULL || value_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "打开命名空间 '%s' 失败: %s", namespace_name, esp_err_to_name(err));
        return err;
    }

    size_t required_size = value_len;
    err = nvs_get_str(nvs_handle, key, value, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "读取键 '%s' 失败: %s", key, esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}

esp_err_t nvs_storage_set_i32(const char *namespace_name, const char *key, int32_t value)
{
    if (!s_inited) {
        ESP_LOGE(TAG, "NVS 未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    if (namespace_name == NULL || key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "打开命名空间 '%s' 失败: %s", namespace_name, esp_err_to_name(err));
        return err;
    }

    err = nvs_set_i32(nvs_handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "设置键 '%s' 失败: %s", key, esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "提交更改失败: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}

esp_err_t nvs_storage_get_i32(const char *namespace_name, const char *key, int32_t *value)
{
    if (!s_inited) {
        ESP_LOGE(TAG, "NVS 未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    if (namespace_name == NULL || key == NULL || value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "打开命名空间 '%s' 失败: %s", namespace_name, esp_err_to_name(err));
        return err;
    }

    err = nvs_get_i32(nvs_handle, key, value);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "读取键 '%s' 失败: %s", key, esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}

esp_err_t nvs_storage_erase_key(const char *namespace_name, const char *key)
{
    if (!s_inited) {
        ESP_LOGE(TAG, "NVS 未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    if (namespace_name == NULL || key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "打开命名空间 '%s' 失败: %s", namespace_name, esp_err_to_name(err));
        return err;
    }

    err = nvs_erase_key(nvs_handle, key);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "删除键 '%s' 失败: %s", key, esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "提交更改失败: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}

esp_err_t nvs_storage_erase_namespace(const char *namespace_name)
{
    if (!s_inited) {
        ESP_LOGE(TAG, "NVS 未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    if (namespace_name == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "打开命名空间 '%s' 失败: %s", namespace_name, esp_err_to_name(err));
        return err;
    }

    err = nvs_erase_all(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "删除命名空间 '%s' 失败: %s", namespace_name, esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "提交更改失败: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}

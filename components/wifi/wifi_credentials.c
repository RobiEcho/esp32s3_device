#include "wifi_credentials.h"
#include "nvs_storage.h"
#include "esp_log.h"
#include "nvs.h"

static const char *TAG = "wifi_credentials";

#define WIFI_STA_NAMESPACE  "wifi_sta"
#define WIFI_AP_NAMESPACE   "wifi_ap"
#define SSID_KEY            "ssid"
#define PASSWORD_KEY        "password"

esp_err_t wifi_credentials_save_sta(const char *ssid, const char *password)
{
    if (ssid == NULL || ssid[0] == '\0') {
        ESP_LOGE(TAG, "SSID 不能为空");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = nvs_storage_set_str(WIFI_STA_NAMESPACE, SSID_KEY, ssid);
    if (err != ESP_OK) {
        return err;
    }

    if (password != NULL) {
        err = nvs_storage_set_str(WIFI_STA_NAMESPACE, PASSWORD_KEY, password);
        if (err != ESP_OK) {
            return err;
        }
    }

    ESP_LOGI(TAG, "STA 凭证已保存");
    return ESP_OK;
}

esp_err_t wifi_credentials_load_sta(char *ssid, size_t ssid_len, char *password, size_t pass_len)
{
    if (ssid == NULL || ssid_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = nvs_storage_get_str(WIFI_STA_NAMESPACE, SSID_KEY, ssid, ssid_len);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "读取 STA SSID 失败");
        return err;
    }

    if (password != NULL && pass_len > 0) {
        err = nvs_storage_get_str(WIFI_STA_NAMESPACE, PASSWORD_KEY, password, pass_len);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "读取 STA 密码失败");
            return err;
        }
    }

    return ESP_OK;
}

esp_err_t wifi_credentials_save_ap(const char *ssid, const char *password)
{
    if (ssid == NULL || ssid[0] == '\0') {
        ESP_LOGE(TAG, "SSID 不能为空");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = nvs_storage_set_str(WIFI_AP_NAMESPACE, SSID_KEY, ssid);
    if (err != ESP_OK) {
        return err;
    }

    if (password != NULL) {
        err = nvs_storage_set_str(WIFI_AP_NAMESPACE, PASSWORD_KEY, password);
        if (err != ESP_OK) {
            return err;
        }
    }

    ESP_LOGI(TAG, "AP 凭证已保存");
    return ESP_OK;
}

esp_err_t wifi_credentials_load_ap(char *ssid, size_t ssid_len, char *password, size_t pass_len)
{
    if (ssid == NULL || ssid_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = nvs_storage_get_str(WIFI_AP_NAMESPACE, SSID_KEY, ssid, ssid_len);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "读取 AP SSID 失败");
        return err;
    }

    if (password != NULL && pass_len > 0) {
        err = nvs_storage_get_str(WIFI_AP_NAMESPACE, PASSWORD_KEY, password, pass_len);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "读取 AP 密码失败");
            return err;
        }
    }

    return ESP_OK;
}

esp_err_t wifi_credentials_erase_sta(void)
{
    return nvs_storage_erase_namespace(WIFI_STA_NAMESPACE);
}

esp_err_t wifi_credentials_erase_ap(void)
{
    return nvs_storage_erase_namespace(WIFI_AP_NAMESPACE);
}

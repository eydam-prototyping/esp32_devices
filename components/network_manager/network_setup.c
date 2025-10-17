#include "network_manager.h"

static const char *TAG = "network_setup.c";

char STA_SSID[32];
char STA_PASSWORD[64];
char STA_BSSID[18];
bool STA_USE_SPECIFIC_BSSID;

char AP_SSID[32];
char AP_PASSWORD[64];
uint8_t AP_CHANNEL;
uint8_t AP_MAX_CONNECTIONS;

void load_string_from_nvs(nvs_handle_t nvs_handle, const char* key, char* out_value, size_t max_len, const char* default_value) {
    size_t required_size = 0;
    esp_err_t err = nvs_get_str(nvs_handle, key, NULL, &required_size);
    if (err == ESP_OK && required_size <= max_len) {
        nvs_get_str(nvs_handle, key, out_value, &required_size);
    } else {
        out_value = strncpy(out_value, default_value, max_len);
        out_value[max_len - 1] = '\0'; // Ensure null-termination
    }
}

void load_i8_from_nvs(nvs_handle_t nvs_handle, const char* key, int8_t* out_value, int8_t default_value) {
    esp_err_t err = nvs_get_i8(nvs_handle, key, out_value);
    if (err != ESP_OK) {
        *out_value = default_value;
    }
}

void load_wifi_config(void){
    // Load Wi-Fi configuration from NVS
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE_WIFI_CONFIG, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle");
        return;
    }

    load_string_from_nvs(nvs_handle, NVS_KEY_STA_SSID, STA_SSID, sizeof(STA_SSID), DEFAULT_WIFI_STA_SSID);
    ESP_LOGI(TAG, "Loaded STA SSID: %s", STA_SSID);

    load_string_from_nvs(nvs_handle, NVS_KEY_STA_PASSWORD, STA_PASSWORD, sizeof(STA_PASSWORD), DEFAULT_WIFI_STA_PASSWORD);
    ESP_LOGI(TAG, "Loaded STA Password: %s", STA_PASSWORD);

    load_string_from_nvs(nvs_handle, NVS_KEY_STA_BSSID, STA_BSSID, sizeof(STA_BSSID), DEFAULT_WIFI_STA_BSSID);
    ESP_LOGI(TAG, "Loaded STA BSSID: %s", STA_BSSID);

    load_i8_from_nvs(nvs_handle, NVS_KEY_STA_USE_SPECIFIC_BSSID, (int8_t*)&STA_USE_SPECIFIC_BSSID, DEFAULT_WIFI_STA_USE_SPECIFIC_BSSID);
    ESP_LOGI(TAG, "Loaded STA Use Specific BSSID: %s", STA_USE_SPECIFIC_BSSID ? "true" : "false");

    load_string_from_nvs(nvs_handle, NVS_KEY_AP_SSID, AP_SSID, sizeof(AP_SSID), DEFAULT_WIFI_AP_SSID);
    ESP_LOGI(TAG, "Loaded AP SSID: %s", AP_SSID);

    load_string_from_nvs(nvs_handle, NVS_KEY_AP_PASSWORD, AP_PASSWORD, sizeof(AP_PASSWORD), DEFAULT_WIFI_AP_PASSWORD);
    ESP_LOGI(TAG, "Loaded AP Password: %s", AP_PASSWORD);
    
    load_i8_from_nvs(nvs_handle, NVS_KEY_AP_CHANNEL, (int8_t*)&AP_CHANNEL, DEFAULT_WIFI_AP_CHANNEL);
    ESP_LOGI(TAG, "Loaded AP Channel: %d", AP_CHANNEL);

    load_i8_from_nvs(nvs_handle, NVS_KEY_AP_MAX_CONNECTIONS, (int8_t*)&AP_MAX_CONNECTIONS, DEFAULT_WIFI_AP_MAX_CONNECTIONS);
    ESP_LOGI(TAG, "Loaded AP Max Connections: %d", AP_MAX_CONNECTIONS);

    nvs_close(nvs_handle);
}

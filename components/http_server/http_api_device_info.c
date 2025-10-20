#include "http_server.h"
#include "esp_chip_info.h"
#include "sdkconfig.h"
#include "git_version.h"
#include "esp_mac.h"
#include "esp_partition.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_image_format.h"
#include "esp_flash.h"
#include "esp_ota_ops.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/rtc.h"

static const char *TAG = "http_api_device_info.c";

/* Get device information as JSON
 * Endpoint: /api/device/info
 */
char *get_device_info(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        ESP_LOGE(TAG, "Failed to create root JSON object");
        return NULL;
    }
    // Device information
    const char *device_name = "ESP32_Device";
    const char *device_model = CONFIG_IDF_TARGET;
    const char *device_arch = CONFIG_IDF_TARGET_ARCH;
    const char *firmware_version = "0.0.1";
    const char *sdk_version = esp_get_idf_version();

    cJSON_AddStringToObject(root, "device_name", device_name);
    cJSON_AddStringToObject(root, "device_model", device_model);
    cJSON_AddStringToObject(root, "device_arch", device_arch);
    cJSON_AddStringToObject(root, "firmware_version", firmware_version);
    cJSON_AddStringToObject(root, "sdk_version", sdk_version);

    // Add Git version information
    cJSON *git_info = cJSON_CreateObject();
    if (git_info != NULL)
    {
        cJSON_AddStringToObject(git_info, "branch", GIT_BRANCH);
        cJSON_AddStringToObject(git_info, "commit", GIT_COMMIT_HASH);
        cJSON_AddStringToObject(git_info, "version", GIT_VERSION);
        cJSON_AddStringToObject(git_info, "commit_date", GIT_COMMIT_DATE);

        // Build information
        char build_timestamp[64];
        snprintf(build_timestamp, sizeof(build_timestamp), "%s %s", BUILD_DATE, BUILD_TIME);
        cJSON_AddStringToObject(git_info, "build_timestamp", build_timestamp);

        // Add dirty flag info
        cJSON_AddBoolToObject(git_info, "clean_build", is_clean_build());

        cJSON_AddItemToObject(root, "git_info", git_info);
    }
    else
    {
        ESP_LOGW(TAG, "Failed to create git_info object");
    }

    // MAC addresses information
    cJSON *mac_addresses = cJSON_CreateObject();
    if (mac_addresses != NULL)
    {
        uint8_t mac[6];
        char mac_str[18];

        // WiFi Station MAC
        esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);
        if (ret == ESP_OK)
        {
            snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            cJSON_AddStringToObject(mac_addresses, "wifi_sta", mac_str);
        }
        else
        {
            cJSON_AddStringToObject(mac_addresses, "wifi_sta", "unknown");
        }

        // WiFi Access Point MAC
        ret = esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
        if (ret == ESP_OK)
        {
            snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            cJSON_AddStringToObject(mac_addresses, "wifi_ap", mac_str);
        }
        else
        {
            cJSON_AddStringToObject(mac_addresses, "wifi_ap", "unknown");
        }

        // Bluetooth MAC (if available)
        ret = esp_read_mac(mac, ESP_MAC_BT);
        if (ret == ESP_OK)
        {
            snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            cJSON_AddStringToObject(mac_addresses, "bluetooth", mac_str);
        }
        else
        {
            cJSON_AddStringToObject(mac_addresses, "bluetooth", "not_available");
        }

        // Ethernet MAC (if available)
        ret = esp_read_mac(mac, ESP_MAC_ETH);
        if (ret == ESP_OK)
        {
            snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            cJSON_AddStringToObject(mac_addresses, "ethernet", mac_str);
        }
        else
        {
            cJSON_AddStringToObject(mac_addresses, "ethernet", "not_available");
        }

        // Base MAC (factory programmed)
        ret = esp_efuse_mac_get_default(mac);
        if (ret == ESP_OK)
        {
            snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            cJSON_AddStringToObject(mac_addresses, "base_mac", mac_str);
        }
        else
        {
            cJSON_AddStringToObject(mac_addresses, "base_mac", "unknown");
        }

        cJSON_AddItemToObject(root, "mac_addresses", mac_addresses);
    }
    else
    {
        ESP_LOGW(TAG, "Failed to create mac_addresses object");
    }

    // CPU Frequency - use simple fallback for ESP-IDF v5.5
    uint32_t cpu_freq_mhz = 240; // Default ESP32 frequency
#ifdef CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ
    cpu_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
#endif
    cJSON_AddNumberToObject(root, "cpu_frequency_mhz", cpu_freq_mhz);

    // Reset Reason
    esp_reset_reason_t reset_reason = esp_reset_reason();
    const char *reset_reason_str = "unknown";
    switch (reset_reason)
    {
    case ESP_RST_POWERON:
        reset_reason_str = "power_on";
        break;
    case ESP_RST_EXT:
        reset_reason_str = "external_reset";
        break;
    case ESP_RST_SW:
        reset_reason_str = "software_reset";
        break;
    case ESP_RST_PANIC:
        reset_reason_str = "panic_reset";
        break;
    case ESP_RST_INT_WDT:
        reset_reason_str = "interrupt_watchdog";
        break;
    case ESP_RST_TASK_WDT:
        reset_reason_str = "task_watchdog";
        break;
    case ESP_RST_WDT:
        reset_reason_str = "other_watchdog";
        break;
    case ESP_RST_DEEPSLEEP:
        reset_reason_str = "deep_sleep";
        break;
    case ESP_RST_BROWNOUT:
        reset_reason_str = "brownout";
        break;
    case ESP_RST_SDIO:
        reset_reason_str = "sdio_reset";
        break;
    default:
        reset_reason_str = "unknown";
        break;
    }
    cJSON_AddStringToObject(root, "reset_reason", reset_reason_str);

    // Chip information
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    cJSON *chip_info_json = cJSON_CreateObject();
    if (chip_info_json == NULL)
    {
        ESP_LOGE(TAG, "Failed to create chip_info_json object");
        cJSON_Delete(root);
        return NULL;
    }

    cJSON_AddNumberToObject(chip_info_json, "cores", chip_info.cores);

    // Add features as an array of strings
    cJSON *features_array = cJSON_CreateArray();
    if (features_array == NULL)
    {
        ESP_LOGE(TAG, "Failed to create features_array");
        cJSON_Delete(chip_info_json);
        cJSON_Delete(root);
        return NULL;
    }

    // Add features with proper error checking
    cJSON *feature_item = NULL;
    if (chip_info.features & CHIP_FEATURE_EMB_FLASH)
    {
        feature_item = cJSON_CreateString("Embedded Flash");
        if (feature_item)
            cJSON_AddItemToArray(features_array, feature_item);
    }
    if (chip_info.features & CHIP_FEATURE_WIFI_BGN)
    {
        feature_item = cJSON_CreateString("WIFI2.4GHz");
        if (feature_item)
            cJSON_AddItemToArray(features_array, feature_item);
    }
    if (chip_info.features & CHIP_FEATURE_BT)
    {
        feature_item = cJSON_CreateString("BT");
        if (feature_item)
            cJSON_AddItemToArray(features_array, feature_item);
    }
    if (chip_info.features & CHIP_FEATURE_BLE)
    {
        feature_item = cJSON_CreateString("BLE");
        if (feature_item)
            cJSON_AddItemToArray(features_array, feature_item);
    }
    if (chip_info.features & CHIP_FEATURE_IEEE802154)
    {
        feature_item = cJSON_CreateString("802.15.4");
        if (feature_item)
            cJSON_AddItemToArray(features_array, feature_item);
    }
    if (chip_info.features & CHIP_FEATURE_EMB_PSRAM)
    {
        feature_item = cJSON_CreateString("Embedded PSRAM");
        if (feature_item)
            cJSON_AddItemToArray(features_array, feature_item);
    }

    cJSON_AddItemToObject(root, "features", features_array);

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    char revision_str[8];
    snprintf(revision_str, sizeof(revision_str), "v%d.%d", major_rev, minor_rev);
    cJSON_AddStringToObject(chip_info_json, "revision", revision_str);
    cJSON_AddItemToObject(root, "chip_info", chip_info_json);

    return return_json_object(root);
}

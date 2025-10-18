#include "http_server.h"
#include "esp_chip_info.h"
#include "sdkconfig.h"
#include "git_version.h"
#include "esp_mac.h"
#include "esp_heap_caps.h"
#include "esp_flash.h"
#include "esp_partition.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"
#include "esp_image_format.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/rtc.h"

static const char *TAG = "http_api_device_info.c";

/* Get device information as JSON
 * Endpoint: /api/device/info
 *
 * Example JSON response:
 * {
 *   "device_name": "ESP32_Device",
 *   "device_model": "esp32",
 *   "device_arch": "xtensa",
 *   "firmware_version": "0.0.1",
 *   "sdk_version": "v5.5.1",
 *   "git_info": {
 *       "branch": "2025-10-18_tests",
 *       "commit": "a1b2c3d",
 *       "version": "a1b2c3d-dirty",
 *       "commit_date": "2025-10-18 15:30:45 +0200",
 *       "build_timestamp": "Oct 18 2025 15:45:12",
 *       "clean_build": false
 *   },
 *   "mac_addresses": {
 *       "wifi_sta": "24:6f:28:aa:bb:cc",
 *       "wifi_ap": "24:6f:28:aa:bb:cd",
 *       "bluetooth": "24:6f:28:aa:bb:ce",
 *       "ethernet": "not_available",
 *       "base_mac": "24:6f:28:aa:bb:cc"
 *   },
 *   "memory_info": {
 *       "free_heap": 234567,
 *       "total_heap": 327680,
 *       "used_heap": 93113,
 *       "largest_free_block": 204800,
 *       "heap_usage_percent": 28.4,
 *       "psram_status": "not_available",
 *       "flash_size": 4194304,
 *       "spiffs_total": 1048576,
 *       "spiffs_used": 123456,
 *       "spiffs_free": 925120,
 *       "spiffs_usage_percent": 11.8
 *   },
 *   "system_info": {
 *       "cpu_frequency_mhz": 240,
 *       "uptime_seconds": 3661,
 *       "uptime_minutes": 61,
 *       "uptime_hours": 1,
 *       "reset_reason": "power_on",
 *       "active_tasks": 12,
 *       "current_task_stack_free": 2048
 *   },
 *   "sensors_info": {
 *       "internal_temperature_celsius": 45.2
 *   },
 *   "hardware_info": {
 *       "crystal_frequency_mhz": 40,
 *       "flash_mode": "DIO",
 *       "flash_speed": "40MHz",
 *       "running_partition": "factory",
 *       "partitions": [
 *           {"label": "nvs", "type": "data", "size": 24576, "address": 36864},
 *           {"label": "phy_init", "type": "data", "size": 4096, "address": 61440},
 *           {"label": "factory", "type": "app", "size": 1048576, "address": 65536}
 *       ]
 *   },
 *   "chip_info": {
 *       "cores": 2,
 *       "features": ["Embedded Flash", "WIFI2.4GHz", "BT", "BLE"],
 *       "revision": "v3.1"
 *   }
 * }
 */
char *get_device_info(httpd_req_t *req){
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
    } else {
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
        if (ret == ESP_OK) {
            snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", 
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            cJSON_AddStringToObject(mac_addresses, "wifi_sta", mac_str);
        } else {
            cJSON_AddStringToObject(mac_addresses, "wifi_sta", "unknown");
        }
        
        // WiFi Access Point MAC
        ret = esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
        if (ret == ESP_OK) {
            snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", 
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            cJSON_AddStringToObject(mac_addresses, "wifi_ap", mac_str);
        } else {
            cJSON_AddStringToObject(mac_addresses, "wifi_ap", "unknown");
        }
        
        // Bluetooth MAC (if available)
        ret = esp_read_mac(mac, ESP_MAC_BT);
        if (ret == ESP_OK) {
            snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", 
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            cJSON_AddStringToObject(mac_addresses, "bluetooth", mac_str);
        } else {
            cJSON_AddStringToObject(mac_addresses, "bluetooth", "not_available");
        }
        
        // Ethernet MAC (if available)
        ret = esp_read_mac(mac, ESP_MAC_ETH);
        if (ret == ESP_OK) {
            snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", 
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            cJSON_AddStringToObject(mac_addresses, "ethernet", mac_str);
        } else {
            cJSON_AddStringToObject(mac_addresses, "ethernet", "not_available");
        }
        
        // Base MAC (factory programmed)
        ret = esp_efuse_mac_get_default(mac);
        if (ret == ESP_OK) {
            snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", 
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            cJSON_AddStringToObject(mac_addresses, "base_mac", mac_str);
        } else {
            cJSON_AddStringToObject(mac_addresses, "base_mac", "unknown");
        }
        
        cJSON_AddItemToObject(root, "mac_addresses", mac_addresses);
    } else {
        ESP_LOGW(TAG, "Failed to create mac_addresses object");
    }

    // Memory & Storage Information
    cJSON *memory_info = cJSON_CreateObject();
    if (memory_info != NULL)
    {
        // Heap Memory
        size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
        size_t total_heap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
        size_t largest_free_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
        
        cJSON_AddNumberToObject(memory_info, "free_heap", free_heap);
        cJSON_AddNumberToObject(memory_info, "total_heap", total_heap);
        cJSON_AddNumberToObject(memory_info, "used_heap", total_heap - free_heap);
        cJSON_AddNumberToObject(memory_info, "largest_free_block", largest_free_block);
        cJSON_AddNumberToObject(memory_info, "heap_usage_percent", 
                               (float)(total_heap - free_heap) / total_heap * 100.0);

        // PSRAM (if available)
        size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        size_t total_psram = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
        
        if (total_psram > 0) {
            cJSON_AddNumberToObject(memory_info, "free_psram", free_psram);
            cJSON_AddNumberToObject(memory_info, "total_psram", total_psram);
            cJSON_AddNumberToObject(memory_info, "used_psram", total_psram - free_psram);
            cJSON_AddNumberToObject(memory_info, "psram_usage_percent", 
                                   (float)(total_psram - free_psram) / total_psram * 100.0);
        } else {
            cJSON_AddStringToObject(memory_info, "psram_status", "not_available");
        }

        // Flash Information
        uint32_t flash_size = 0;
        esp_flash_get_size(NULL, &flash_size);
        cJSON_AddNumberToObject(memory_info, "flash_size", flash_size);

        // SPIFFS Information (if mounted)
        size_t total_spiffs = 0, used_spiffs = 0;
        esp_err_t ret = esp_spiffs_info(NULL, &total_spiffs, &used_spiffs);
        if (ret == ESP_OK) {
            cJSON_AddNumberToObject(memory_info, "spiffs_total", total_spiffs);
            cJSON_AddNumberToObject(memory_info, "spiffs_used", used_spiffs);
            cJSON_AddNumberToObject(memory_info, "spiffs_free", total_spiffs - used_spiffs);
            cJSON_AddNumberToObject(memory_info, "spiffs_usage_percent", 
                                   (float)used_spiffs / total_spiffs * 100.0);
        } else {
            cJSON_AddStringToObject(memory_info, "spiffs_status", "not_mounted");
        }
        
        cJSON_AddItemToObject(root, "memory_info", memory_info);
    } else {
        ESP_LOGW(TAG, "Failed to create memory_info object");
    }

    // System Performance Information
    cJSON *system_info = cJSON_CreateObject();
    if (system_info != NULL)
    {
        // CPU Frequency - use simple fallback for ESP-IDF v5.5
        uint32_t cpu_freq_mhz = 240; // Default ESP32 frequency
        #ifdef CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ
        cpu_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
        #endif
        cJSON_AddNumberToObject(system_info, "cpu_frequency_mhz", cpu_freq_mhz);
        
        // Uptime
        int64_t uptime_us = esp_timer_get_time();
        uint32_t uptime_seconds = uptime_us / 1000000;
        cJSON_AddNumberToObject(system_info, "uptime_seconds", uptime_seconds);
        cJSON_AddNumberToObject(system_info, "uptime_minutes", uptime_seconds / 60);
        cJSON_AddNumberToObject(system_info, "uptime_hours", uptime_seconds / 3600);
        
        // Reset Reason
        esp_reset_reason_t reset_reason = esp_reset_reason();
        const char* reset_reason_str = "unknown";
        switch (reset_reason) {
            case ESP_RST_POWERON: reset_reason_str = "power_on"; break;
            case ESP_RST_EXT: reset_reason_str = "external_reset"; break;
            case ESP_RST_SW: reset_reason_str = "software_reset"; break;
            case ESP_RST_PANIC: reset_reason_str = "panic_reset"; break;
            case ESP_RST_INT_WDT: reset_reason_str = "interrupt_watchdog"; break;
            case ESP_RST_TASK_WDT: reset_reason_str = "task_watchdog"; break;
            case ESP_RST_WDT: reset_reason_str = "other_watchdog"; break;
            case ESP_RST_DEEPSLEEP: reset_reason_str = "deep_sleep"; break;
            case ESP_RST_BROWNOUT: reset_reason_str = "brownout"; break;
            case ESP_RST_SDIO: reset_reason_str = "sdio_reset"; break;
            default: reset_reason_str = "unknown"; break;
        }
        cJSON_AddStringToObject(system_info, "reset_reason", reset_reason_str);
        
        // Task Count
        UBaseType_t task_count = uxTaskGetNumberOfTasks();
        cJSON_AddNumberToObject(system_info, "active_tasks", task_count);
        
        // Stack High Water Mark of current task
        UBaseType_t stack_hwm = uxTaskGetStackHighWaterMark(NULL);
        cJSON_AddNumberToObject(system_info, "current_task_stack_free", stack_hwm * sizeof(StackType_t));
        
        cJSON_AddItemToObject(root, "system_info", system_info);
    } else {
        ESP_LOGW(TAG, "Failed to create system_info object");
    }

    // Hardware Sensors Information
    cJSON *sensors_info = cJSON_CreateObject();
    if (sensors_info != NULL)
    {
        // Internal Temperature Sensor - not available in this ESP-IDF version
        cJSON_AddStringToObject(sensors_info, "internal_temperature", "not_supported_in_idf_v5_5");
        
        cJSON_AddItemToObject(root, "sensors_info", sensors_info);
    } else {
        ESP_LOGW(TAG, "Failed to create sensors_info object");
    }

    // Additional Hardware Information
    cJSON *hardware_info = cJSON_CreateObject();
    if (hardware_info != NULL)
    {
        // Crystal Frequency
        uint32_t xtal_freq = rtc_clk_xtal_freq_get();
        cJSON_AddNumberToObject(hardware_info, "crystal_frequency_mhz", xtal_freq);
        
        // Flash Configuration
        esp_image_header_t image_header;
        const esp_partition_t* running_partition = esp_ota_get_running_partition();
        if (running_partition != NULL) {
            // Flash mode and speed from image header
            if (esp_partition_read(running_partition, 0, &image_header, sizeof(image_header)) == ESP_OK) {
                const char* flash_mode = "unknown";
                switch (image_header.spi_mode) {
                    case ESP_IMAGE_SPI_MODE_QIO: flash_mode = "QIO"; break;
                    case ESP_IMAGE_SPI_MODE_QOUT: flash_mode = "QOUT"; break;
                    case ESP_IMAGE_SPI_MODE_DIO: flash_mode = "DIO"; break;
                    case ESP_IMAGE_SPI_MODE_DOUT: flash_mode = "DOUT"; break;
                    default: flash_mode = "unknown"; break;
                }
                cJSON_AddStringToObject(hardware_info, "flash_mode", flash_mode);
                
                const char* flash_speed = "unknown";
                switch (image_header.spi_speed) {
                    case ESP_IMAGE_SPI_SPEED_DIV_1: flash_speed = "80MHz"; break;
                    case ESP_IMAGE_SPI_SPEED_DIV_2: flash_speed = "40MHz"; break;
                    case ESP_IMAGE_SPI_SPEED_DIV_3: flash_speed = "26MHz"; break;
                    case ESP_IMAGE_SPI_SPEED_DIV_4: flash_speed = "20MHz"; break;
                    default: flash_speed = "unknown"; break;
                }
                cJSON_AddStringToObject(hardware_info, "flash_speed", flash_speed);
            }
            
            cJSON_AddStringToObject(hardware_info, "running_partition", running_partition->label);
        }
        
        // Partition Table Information
        cJSON *partitions_array = cJSON_CreateArray();
        if (partitions_array != NULL) {
            esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
            if (it != NULL) {
                while (it != NULL) {
                    const esp_partition_t* partition = esp_partition_get(it);
                    if (partition != NULL) {
                        cJSON *partition_obj = cJSON_CreateObject();
                        if (partition_obj != NULL) {
                            cJSON_AddStringToObject(partition_obj, "label", partition->label);
                            
                            const char* type_str = "unknown";
                            switch (partition->type) {
                                case ESP_PARTITION_TYPE_APP: type_str = "app"; break;
                                case ESP_PARTITION_TYPE_DATA: type_str = "data"; break;
                                default: type_str = "unknown"; break;
                            }
                            cJSON_AddStringToObject(partition_obj, "type", type_str);
                            cJSON_AddNumberToObject(partition_obj, "size", partition->size);
                            cJSON_AddNumberToObject(partition_obj, "address", partition->address);
                            
                            cJSON_AddItemToArray(partitions_array, partition_obj);
                        } else {
                            ESP_LOGW(TAG, "Failed to create partition object");
                        }
                    }
                    it = esp_partition_next(it);
                }
                esp_partition_iterator_release(it);
            } else {
                ESP_LOGW(TAG, "No partitions found");
            }
            
            cJSON_AddItemToObject(hardware_info, "partitions", partitions_array);
        } else {
            ESP_LOGW(TAG, "Failed to create partitions_array");
        }
        
        cJSON_AddItemToObject(root, "hardware_info", hardware_info);
    } else {
        ESP_LOGW(TAG, "Failed to create hardware_info object");
    }

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
    if (chip_info.features & CHIP_FEATURE_EMB_FLASH) {
        feature_item = cJSON_CreateString("Embedded Flash");
        if (feature_item) cJSON_AddItemToArray(features_array, feature_item);
    } 
    if (chip_info.features & CHIP_FEATURE_WIFI_BGN) {
        feature_item = cJSON_CreateString("WIFI2.4GHz");
        if (feature_item) cJSON_AddItemToArray(features_array, feature_item);
    }
    if (chip_info.features & CHIP_FEATURE_BT) {
        feature_item = cJSON_CreateString("BT");
        if (feature_item) cJSON_AddItemToArray(features_array, feature_item);
    }
    if (chip_info.features & CHIP_FEATURE_BLE) {
        feature_item = cJSON_CreateString("BLE");
        if (feature_item) cJSON_AddItemToArray(features_array, feature_item);
    }
    if (chip_info.features & CHIP_FEATURE_IEEE802154) {
        feature_item = cJSON_CreateString("802.15.4");
        if (feature_item) cJSON_AddItemToArray(features_array, feature_item);
    }
    if (chip_info.features & CHIP_FEATURE_EMB_PSRAM) {
        feature_item = cJSON_CreateString("Embedded PSRAM");
        if (feature_item) cJSON_AddItemToArray(features_array, feature_item);
    }
    
    cJSON_AddItemToObject(chip_info_json, "features", features_array);
    
    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    char revision_str[8];
    snprintf(revision_str, sizeof(revision_str), "v%d.%d", major_rev, minor_rev);
    cJSON_AddStringToObject(chip_info_json, "revision", revision_str);
    cJSON_AddItemToObject(root, "chip_info", chip_info_json);

    return return_json_object(root);
}
#include "http_server.h"
#include "esp_chip_info.h"
#include "sdkconfig.h"

static const char *TAG = "http_api_device_info.c";

/* Get device information as JSON
 * Endpoint: /api/device/info
 *
 * Example JSON response:
 * {
 *   "device_name": "ESP32_Device",
 *   "device_model": "ESP32-WROOM-32",
 *   "firmware_version": "1.0.0",
 *   "sdk_version": "v4.4",
 *   "chip_info": {
 *       "cores": 2,
 *       "features": ["WIFI", "BT", "BLE"],
 *       "revision": 1,
 *       "model": "ESP32-WROOM-32"
 *   }
 * }
 */
char *get_device_info(httpd_req_t *req){
    // Create a JSON object
    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    // Device information
    const char *device_name = "ESP32_Device"; // You can customize this
    const char *device_model = CONFIG_IDF_TARGET; // e.g., "ESP32-WROOM-32"
    const char *firmware_version = "1.0.0"; // You can customize this
    const char *sdk_version = esp_get_idf_version();

    cJSON_AddStringToObject(root, "device_name", device_name);
    cJSON_AddStringToObject(root, "device_model", device_model);
    cJSON_AddStringToObject(root, "firmware_version", firmware_version);
    cJSON_AddStringToObject(root, "sdk_version", sdk_version);

    // Chip information
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    cJSON *chip_info_json = cJSON_CreateObject();
    if (chip_info_json == NULL)
    {
        cJSON_Delete(root);
        return NULL;
    }

    cJSON_AddNumberToObject(chip_info_json, "cores", chip_info.cores);
    
    // Add features as an array of strings
    cJSON *features_array = cJSON_CreateArray();
    if (features_array == NULL)
    {
        cJSON_Delete(root);
        cJSON_Delete(chip_info_json);
        return NULL;
    }

    if (chip_info.features & CHIP_FEATURE_WIFI_BGN) {
        cJSON_AddItemToArray(features_array, cJSON_CreateString("WIFI"));
    }
    if (chip_info.features & CHIP_FEATURE_BT) {
        cJSON_AddItemToArray(features_array, cJSON_CreateString("BT"));
    }
    if (chip_info.features & CHIP_FEATURE_BLE) {
        cJSON_AddItemToArray(features_array, cJSON_CreateString("BLE"));
    }
    if (chip_info.features & CHIP_FEATURE_IEEE802154) {
        cJSON_AddItemToArray(features_array, cJSON_CreateString("802.15.4"));
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
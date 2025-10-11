#include "http_server.h"

static const char *TAG = "http_api_config";


/* Get WiFi AP configuration
    * Endpoint: /api/wifi/config/ap
    *
    * Example JSON response:
    * {
    *   "success": true,
    *   "status": "WiFi AP config retrieved",
    *   "ssid": "Your_AP_SSID",
    *   "password": "Your_AP_Password",
    *   "channel": 6,
    *   "max_connections": 4
    * }
    */
char *get_wifi_config_ap(httpd_req_t *req)
{
    // Create a JSON object
    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE_WIFI_CONFIG, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "status", "Failed to open NVS handle");
        return return_json_object(root);
    }

    char *ap_ssid = (char *)malloc(32);
    char *ap_password = (char *)malloc(64);
    int8_t ap_channel = 0;
    int8_t ap_max_connections = 0;

    // Read WiFi AP configuration from NVS
    size_t ap_ssid_len = 32;
    size_t ap_password_len = 64;

    err = nvs_get_str(nvs_handle, NVS_KEY_AP_SSID, ap_ssid, &ap_ssid_len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Error (%s) getting SSID from NVS!", esp_err_to_name(err));
    }

    err = nvs_get_str(nvs_handle, NVS_KEY_AP_PASSWORD, ap_password, &ap_password_len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Error (%s) getting Password from NVS!", esp_err_to_name(err));
    }

    err = nvs_get_i8(nvs_handle, NVS_KEY_AP_CHANNEL, &ap_channel);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Error (%s) getting AP channel from NVS!", esp_err_to_name(err));
    }

    err = nvs_get_i8(nvs_handle, NVS_KEY_AP_MAX_CONNECTIONS, &ap_max_connections);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Error (%s) getting AP max connections from NVS!", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);

    cJSON_AddBoolToObject(root, "success", true);
    cJSON_AddStringToObject(root, "status", "WiFi AP config retrieved");
    cJSON_AddStringToObject(root, "ssid", ap_ssid);
    cJSON_AddStringToObject(root, "password", ap_password);
    cJSON_AddNumberToObject(root, "channel", ap_channel);
    cJSON_AddNumberToObject(root, "max_connections", ap_max_connections);

    return return_json_object(root);
}



/* Post WiFi configuration for AP
 * Endpoint: /api/wifi/config/ap
 *
 * Expected JSON request body:
 * {
 *   "ssid": "Your_AP_SSID",
 *   "password": "Your_AP_Password",
 *   "channel": 6,                    // Optional
 *   "max_connections": 4             // Optional
 * }
 *
 * Example JSON response:
 * {
 *   "success": true,
 *   "status": "wifi_config_changed"
 * }
 */
char *post_wifi_config_ap(httpd_req_t *req)
{
    ESP_LOGI(TAG, "post_wifi_config_ap");
    // Read the request body
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return NULL;
    }
    buf[ret] = '\0'; // Null-terminate the received data

    ESP_LOGI(TAG, "%s", buf);

    // Parse the JSON data
    cJSON *json = cJSON_Parse(buf);
    if (json == NULL)
    {
        return NULL;
    }

    cJSON *root = cJSON_CreateObject();

    const cJSON *ap_ssid = cJSON_GetObjectItemCaseSensitive(json, "ssid");
    const cJSON *ap_password = cJSON_GetObjectItemCaseSensitive(json, "password");
    const cJSON *ap_channel = cJSON_GetObjectItemCaseSensitive(json, "channel");
    const cJSON *ap_max_connections = cJSON_GetObjectItemCaseSensitive(json, "max_connections");
    if (!cJSON_IsString(ap_ssid) || (ap_ssid->valuestring == NULL) || strlen(ap_ssid->valuestring) == 0)
    {
        ESP_LOGW(TAG, "Wifi Config error: AP SSID must be a non-empty string");
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "status", "AP SSID must be a non-empty string");
        return return_json_object(root);
    }
    if (!cJSON_IsString(ap_password) || (ap_password->valuestring == NULL))
    {
        ESP_LOGW(TAG, "Wifi Config error: AP Password must be a string");
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "status", "AP Password must be a string");
        return return_json_object(root);
    }
    if (ap_channel != NULL && (!cJSON_IsNumber(ap_channel) || (ap_channel->valueint < 1 || ap_channel->valueint > 11)))
    {
        ESP_LOGW(TAG, "Wifi Config error: AP Channel must be a number between 1 and 11");
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "status", "AP Channel must be a number between 1 and 11");
        return return_json_object(root);
    }
    if (ap_max_connections != NULL && (!cJSON_IsNumber(ap_max_connections) || (ap_max_connections->valueint < 1 || ap_max_connections->valueint > 10)))
    {
        ESP_LOGW(TAG, "Wifi Config error: AP Max Connections must be a number between 1 and 10");
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "status", "AP Max Connections must be a number between 1 and 10");
        return return_json_object(root);
    }

    ESP_LOGI(TAG, "Received WiFi AP config: SSID=%s, Password=%s, Channel=%d, Max Connections=%d",
             ap_ssid->valuestring, ap_password->valuestring,
             ap_channel ? ap_channel->valueint : 6, ap_max_connections ? ap_max_connections->valueint : 4);

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE_WIFI_CONFIG, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "status", "Failed to open NVS handle");
        return return_json_object(root);
    }

    // Write the new WiFi AP configuration to NVS
    nvs_set_str(nvs_handle, NVS_KEY_AP_SSID, ap_ssid->valuestring);
    nvs_set_str(nvs_handle, NVS_KEY_AP_PASSWORD, ap_password->valuestring);
    nvs_set_i8(nvs_handle, NVS_KEY_AP_CHANNEL, ap_channel ? ap_channel->valueint : 6);
    nvs_set_i8(nvs_handle, NVS_KEY_AP_MAX_CONNECTIONS, ap_max_connections ? ap_max_connections->valueint : 4);

    // Commit the changes
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) committing NVS changes!", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);

    cJSON_AddBoolToObject(root, "success", true);
    cJSON_AddStringToObject(root, "status", "wifi_config_changed");
    return return_json_object(root);
}


/* Get WiFi configuration for STA
 * Endpoint: /api/wifi/config/sta
 *
 * Example JSON response:
 * {
 *   "success": true,
 *   "status": "WiFi STA config retrieved",
 *   "ssid": "Your_STA_SSID",
 *   "password": "Your_STA_Password",
 *   "bssid": "00:11:22:33:44:55",    // Optional
 *   "use_specific_bssid": false      // Optional
 * }
 */
char *get_wifi_config_sta(httpd_req_t *req)
{
    // Create a JSON object
    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE_WIFI_CONFIG, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "status", "Failed to open NVS handle");
        return return_json_object(root);
    }

    char *sta_ssid = (char *)malloc(32);
    char *sta_password = (char *)malloc(64);
    char *sta_bssid = (char *)malloc(18);
    int8_t sta_use_specific_bssid = 0;

    // Read WiFi STA configuration from NVS
    size_t sta_ssid_len = 32;
    size_t sta_password_len = 64;
    size_t sta_bssid_len = 18;

    err = nvs_get_str(nvs_handle, NVS_KEY_STA_SSID, sta_ssid, &sta_ssid_len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Error (%s) getting SSID from NVS!", esp_err_to_name(err));
    }
    ESP_LOGI(TAG, "STA SSID length: %d", sta_ssid_len);
    ESP_LOGI(TAG, "STA SSID: %s", sta_ssid);

    err = nvs_get_str(nvs_handle, NVS_KEY_STA_PASSWORD, sta_password, &sta_password_len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Error (%s) getting Password from NVS!", esp_err_to_name(err));
    }

    err = nvs_get_str(nvs_handle, NVS_KEY_STA_BSSID, sta_bssid, &sta_bssid_len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Error (%s) getting BSSID length from NVS!", esp_err_to_name(err));
    }

    err = nvs_get_i8(nvs_handle, NVS_KEY_STA_USE_SPECIFIC_BSSID, &sta_use_specific_bssid);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Error (%s) getting Use Specific BSSID from NVS!", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);

    cJSON_AddBoolToObject(root, "success", true);
    cJSON_AddStringToObject(root, "status", "WiFi STA config retrieved");
    cJSON_AddStringToObject(root, "ssid", sta_ssid);
    cJSON_AddStringToObject(root, "password", sta_password);
    cJSON_AddStringToObject(root, "bssid", sta_bssid);
    cJSON_AddBoolToObject(root, "use_specific_bssid", sta_use_specific_bssid);

    return return_json_object(root);
}


/* Post WiFi configuration for STA
 * Endpoint: /api/wifi/config/sta
 *
 * Expected JSON request body:
 * {
 *   "ssid": "Your_STA_SSID",
 *   "password": "Your_STA_Password",
 *   "bssid": "00:11:22:33:44:55",    // Optional
 *   "use_specific_bssid": false      // Optional
 * }
 *
 * Example JSON response:
 * {
 *   "success": true,
 *   "status": "wifi_config_changed"
 * }
 */
char *post_wifi_config_sta(httpd_req_t *req)
{
    ESP_LOGI(TAG, "post_wifi_config_sta");
    // Read the request body
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return NULL;
    }
    buf[ret] = '\0'; // Null-terminate the received data

    ESP_LOGI(TAG, "%s", buf);

    // Parse the JSON data
    cJSON *json = cJSON_Parse(buf);
    if (json == NULL)
    {
        return NULL;
    }

    cJSON *root = cJSON_CreateObject();

    const cJSON *sta_ssid = cJSON_GetObjectItemCaseSensitive(json, "ssid");
    const cJSON *sta_password = cJSON_GetObjectItemCaseSensitive(json, "password");
    const cJSON *sta_bssid = cJSON_GetObjectItemCaseSensitive(json, "bssid");
    const cJSON *sta_use_specific_bssid = cJSON_GetObjectItemCaseSensitive(json, "use_specific_bssid");
    if (!cJSON_IsString(sta_ssid) || (sta_ssid->valuestring == NULL) || strlen(sta_ssid->valuestring) == 0)
    {
        ESP_LOGW(TAG, "Wifi Config error: STA SSID must be a non-empty string");
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "status", "STA SSID must be a non-empty string");
        return return_json_object(root);
    }
    if (!cJSON_IsString(sta_password) || (sta_password->valuestring == NULL))
    {
        ESP_LOGW(TAG, "Wifi Config error: STA Password must be a string");
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "status", "STA Password must be a string");
        return return_json_object(root);
    }
    // If BSSID is provided, it must be a valid string (17 characters for MAC address) or null
    if (sta_bssid != NULL &&    // BSSID is set
        (
            (cJSON_IsString(sta_bssid) && strlen(sta_bssid->valuestring) != 17) ||  // Check for valid MAC address length
            (!cJSON_IsNull(sta_bssid) && !cJSON_IsString(sta_bssid))    // Check for null
        ))
    {
        ESP_LOGW(TAG, "Wifi Config error: STA BSSID must be a string or null");
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "status", "STA BSSID must be a string or null");
        return return_json_object(root);
    }
    if (sta_use_specific_bssid != NULL && !cJSON_IsBool(sta_use_specific_bssid))
    {
        ESP_LOGW(TAG, "Wifi Config error: STA Use Specific BSSID must be a boolean");
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "status", "STA Use Specific BSSID must be a boolean");
        return return_json_object(root);
    }

    ESP_LOGI(TAG, "Received WiFi config: SSID=%s, Password=%s, BSSID=%s, Use Specific BSSID=%s",
             sta_ssid->valuestring, sta_password->valuestring,
             (sta_bssid != NULL && !cJSON_IsNull(sta_bssid)) ? sta_bssid->valuestring : "NULL",
             sta_use_specific_bssid != NULL ? (sta_use_specific_bssid->valueint ? "true" : "false") : "false");

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE_WIFI_CONFIG, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "status", "Failed to open NVS handle");
        return return_json_object(root);
    }
    // Write the new WiFi STA configuration to NVS
    nvs_set_str(nvs_handle, NVS_KEY_STA_SSID, sta_ssid->valuestring);
    nvs_set_str(nvs_handle, NVS_KEY_STA_PASSWORD, sta_password->valuestring);
    nvs_set_str(nvs_handle, NVS_KEY_STA_BSSID, sta_bssid ? sta_bssid->valuestring : NULL);
    nvs_set_i32(nvs_handle, NVS_KEY_STA_USE_SPECIFIC_BSSID, sta_use_specific_bssid ? sta_use_specific_bssid->valueint : 0);

    // Commit the changes
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) committing NVS changes!", esp_err_to_name(err));
    }
    nvs_close(nvs_handle);
    cJSON_AddBoolToObject(root, "success", true);
    cJSON_AddStringToObject(root, "status", "wifi_config_changed");
    return return_json_object(root);
}
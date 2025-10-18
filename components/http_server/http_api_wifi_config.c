#include "http_server.h"

static const char *TAG = "http_api_config";


/* Get WiFi AP configuration
    * Endpoint: /api/wifi/config/ap
    */
char *get_wifi_config_ap(httpd_req_t *req)
{
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
        cJSON_AddStringToObject(root, "error", "Failed to open NVS handle");
        return return_json_object(root);
    }

    char *ap_ssid = (char *)malloc(32);
    char *ap_password = (char *)malloc(64);
    char *ap_hostname = (char *)malloc(32);
    int8_t ap_channel = 0;
    int8_t ap_max_connections = 0;

    size_t ap_ssid_len = 32;
    size_t ap_password_len = 64;
    size_t ap_hostname_len = 32;

    err = nvs_get_str(nvs_handle, NVS_KEY_AP_SSID, ap_ssid, &ap_ssid_len);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Error (%s) getting SSID from NVS, returning default", esp_err_to_name(err));
        strncpy(ap_ssid, DEFAULT_WIFI_AP_SSID, 32);
    }

    err = nvs_get_str(nvs_handle, NVS_KEY_AP_PASSWORD, ap_password, &ap_password_len);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Error (%s) getting password from NVS, returning default", esp_err_to_name(err));
        strncpy(ap_password, DEFAULT_WIFI_AP_PASSWORD, 64);
    }

    err = nvs_get_i8(nvs_handle, NVS_KEY_AP_CHANNEL, &ap_channel);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Error (%s) getting channel from NVS, returning default", esp_err_to_name(err));
        ap_channel = DEFAULT_WIFI_AP_CHANNEL;
    }

    err = nvs_get_i8(nvs_handle, NVS_KEY_AP_MAX_CONNECTIONS, &ap_max_connections);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Error (%s) getting max_connections from NVS, returning default", esp_err_to_name(err));
        ap_max_connections = DEFAULT_WIFI_AP_MAX_CONNECTIONS;
    }

    err = nvs_get_str(nvs_handle, NVS_KEY_AP_HOSTNAME, ap_hostname, &ap_hostname_len);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Error (%s) getting hostname from NVS, returning default", esp_err_to_name(err));
        strncpy(ap_hostname, DEFAULT_WIFI_AP_HOSTNAME, 32);
    }

    nvs_close(nvs_handle);

    cJSON_AddStringToObject(root, "ssid", ap_ssid);
    cJSON_AddStringToObject(root, "password", ap_password);
    cJSON_AddNumberToObject(root, "channel", ap_channel);
    cJSON_AddNumberToObject(root, "max_connections", ap_max_connections);
    cJSON_AddStringToObject(root, "hostname", ap_hostname);

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
 *   "success": true
 * }
 */
char *post_wifi_config_ap(httpd_req_t *req)
{
    ESP_LOGI(TAG, "post_wifi_config_ap");
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
    const cJSON *ap_hostname = cJSON_GetObjectItemCaseSensitive(json, "hostname");

    if (ap_ssid != NULL && ( // fail, if ssid is provided but 
        !cJSON_IsString(ap_ssid) || // not a string or
        (ap_ssid->valuestring == NULL) || // null or
        (strlen(ap_ssid->valuestring) == 0) || // empty or
        (strlen(ap_ssid->valuestring) > 31)))  // longer than 31 characters
    {
        ESP_LOGW(TAG, "Wifi Config error: ssid must be a non-empty string, max 31 characters");
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "error", "ssid must be a non-empty string, max 31 characters");
        return return_json_object(root);
    }
    if (ap_password != NULL && ( // fail, if password is provided but
        !cJSON_IsString(ap_password) || // not a string or
        (ap_password->valuestring == NULL) || // null or
        (strlen(ap_password->valuestring) > 63))) // longer than 63 characters
    {
        ESP_LOGW(TAG, "Wifi Config error: password must be a string, max 63 characters");
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "error", "password must be a string, max 63 characters");
        return return_json_object(root);
    }
    if (ap_hostname != NULL && ( // fail, if hostname is provided but
        !cJSON_IsString(ap_hostname) || // not a string or
        (ap_hostname->valuestring == NULL) || // null or
        (strlen(ap_hostname->valuestring) > 31))) // longer than 31 characters
    {
        ESP_LOGW(TAG, "Wifi Config error: hostname must be a string, max 31 characters");
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "error", "hostname must be a string, max 31 characters");
        return return_json_object(root);
    }
    if (ap_channel != NULL && ( // fail, if channel is provided but
        !cJSON_IsNumber(ap_channel) || // not a number or
        (ap_channel->valueint < 0) || // less than 0 or
        (ap_channel->valueint > 11))) // greater than 11
    {
        ESP_LOGW(TAG, "Wifi Config error: channel must be a number between 0 and 11");
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "error", "channel must be a number between 0 and 11");
        return return_json_object(root);
    }
    if (ap_max_connections != NULL && ( // fail, if max_connections is provided but
        !cJSON_IsNumber(ap_max_connections) || // not a number or
        (ap_max_connections->valueint < 1) || // less than 1 or
        (ap_max_connections->valueint > 10))) // greater than 10
    {
        ESP_LOGW(TAG, "Wifi Config error: max_connections must be a number between 1 and 10");
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "error", "max_connections must be a number between 1 and 10");
        return return_json_object(root);
    }

    ESP_LOGI(TAG, "Received WiFi AP config: SSID=%s, Password=%s, Channel=%d, Max Connections=%d, Hostname=%s",
             ap_ssid ? ap_ssid->valuestring : "N/A", 
             ap_password ? ap_password->valuestring : "N/A",
             ap_channel ? ap_channel->valueint : -1, 
             ap_max_connections ? ap_max_connections->valueint : -1,
             ap_hostname ? ap_hostname->valuestring : "N/A");

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE_WIFI_CONFIG, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "error", "Failed to open NVS handle");
        return return_json_object(root);
    }

    // Write the new WiFi AP configuration to NVS
    if (ap_ssid != NULL) {
        ESP_LOGI(TAG, "Setting AP SSID to: %s", ap_ssid->valuestring);
        nvs_set_str(nvs_handle, NVS_KEY_AP_SSID, ap_ssid->valuestring);
    }

    if (ap_password != NULL) {
        ESP_LOGI(TAG, "Setting AP Password to: %s", ap_password->valuestring);
        nvs_set_str(nvs_handle, NVS_KEY_AP_PASSWORD, ap_password->valuestring);
    }

    if (ap_channel != NULL) {
        ESP_LOGI(TAG, "Setting AP Channel to: %d", ap_channel->valueint);
        nvs_set_i8(nvs_handle, NVS_KEY_AP_CHANNEL, ap_channel->valueint);
    }

    if (ap_max_connections != NULL) {
        ESP_LOGI(TAG, "Setting AP Max Connections to: %d", ap_max_connections->valueint);
        nvs_set_i8(nvs_handle, NVS_KEY_AP_MAX_CONNECTIONS, ap_max_connections->valueint);
    }

    if (ap_hostname != NULL) {
        ESP_LOGI(TAG, "Setting AP Hostname to: %s", ap_hostname->valuestring);
        nvs_set_str(nvs_handle, NVS_KEY_AP_HOSTNAME, ap_hostname->valuestring);
    }

    // Commit the changes
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) committing NVS changes!", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);

    cJSON_AddBoolToObject(root, "success", true);
    return return_json_object(root);
}


/* Get WiFi configuration for STA
 * Endpoint: /api/wifi/config/sta
 *
 * Example JSON response:
 * {
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
        cJSON_AddStringToObject(root, "error", "Failed to open NVS handle");
        return return_json_object(root);
    }

    char *sta_ssid = (char *)malloc(32);
    char *sta_password = (char *)malloc(64);
    char *sta_hostname = (char *)malloc(32);
    char *sta_bssid = (char *)malloc(18);
    int8_t sta_use_specific_bssid = 0;

    // Read WiFi STA configuration from NVS
    size_t sta_ssid_len = 32;
    size_t sta_password_len = 64;
    size_t sta_bssid_len = 18;
    size_t sta_hostname_len = 32;

    err = nvs_get_str(nvs_handle, NVS_KEY_STA_SSID, sta_ssid, &sta_ssid_len);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Error (%s) getting SSID from NVS, returning default", esp_err_to_name(err));
        strncpy(sta_ssid, DEFAULT_WIFI_STA_SSID, 32);
    }
    
    #if BUILD_TYPE == DEVELOP
    err = nvs_get_str(nvs_handle, NVS_KEY_STA_PASSWORD, sta_password, &sta_password_len);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Error (%s) getting Password from NVS, returning default", esp_err_to_name(err));
        strncpy(sta_password, DEFAULT_WIFI_STA_PASSWORD, 64);
    }
    #endif

    err = nvs_get_str(nvs_handle, NVS_KEY_STA_HOSTNAME, sta_hostname, &sta_hostname_len);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Error (%s) getting Hostname from NVS, returning default", esp_err_to_name(err));
        strncpy(sta_hostname, DEFAULT_WIFI_STA_HOSTNAME, 32);
    }

    err = nvs_get_str(nvs_handle, NVS_KEY_STA_BSSID, sta_bssid, &sta_bssid_len);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Error (%s) getting BSSID length from NVS, returning default", esp_err_to_name(err));
        strncpy(sta_bssid, "", 18);
    }

    err = nvs_get_i8(nvs_handle, NVS_KEY_STA_USE_SPECIFIC_BSSID, &sta_use_specific_bssid);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Error (%s) getting use_specific_bssid from NVS, returning default", esp_err_to_name(err));
        sta_use_specific_bssid = DEFAULT_WIFI_STA_USE_SPECIFIC_BSSID;
    }

    nvs_close(nvs_handle);

    cJSON_AddStringToObject(root, "ssid", sta_ssid);
    #if BUILD_TYPE == DEVELOP
    cJSON_AddStringToObject(root, "password", sta_password);
    #endif
    cJSON_AddStringToObject(root, "bssid", sta_bssid);
    cJSON_AddBoolToObject(root, "use_specific_bssid", sta_use_specific_bssid);
    cJSON_AddStringToObject(root, "hostname", sta_hostname);

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
    const cJSON *sta_hostname = cJSON_GetObjectItemCaseSensitive(json, "hostname");

    if (sta_ssid != NULL && ( // fail, if ssid is provided but
        !cJSON_IsString(sta_ssid) || // not a string or
        (sta_ssid->valuestring == NULL) || // null or
        (strlen(sta_ssid->valuestring) == 0) || // empty or
        (strlen(sta_ssid->valuestring) > 31))) // longer than 31 characters
    {
        ESP_LOGW(TAG, "Wifi Config error: SSID must be a non-empty string, max 31 characters");
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "status", "SSID must be a non-empty string, max 31 characters");
        return return_json_object(root);
    }
    if (sta_password != NULL && ( // fail, if password is provided but
        !cJSON_IsString(sta_password) || // not a string or
        (sta_password->valuestring == NULL) || // null or
        (strlen(sta_password->valuestring) > 63))) // longer than 63 characters
    {
        ESP_LOGW(TAG, "Wifi Config error: password must be a string, max 63 characters");
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "status", "password must be a string, max 63 characters");
        return return_json_object(root);
    }
    if (sta_hostname != NULL && ( // fail, if hostname is provided but
        !cJSON_IsString(sta_hostname) || // not a string or
        (sta_hostname->valuestring == NULL) || // null or
        (strlen(sta_hostname->valuestring) > 31))) // longer than 31 characters
    {
        ESP_LOGW(TAG, "Wifi Config error: hostname must be a string, max 31 characters");
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "status", "hostname must be a string, max 31 characters");
        return return_json_object(root);
    }
    if (sta_bssid != NULL && ( // fail, if bssid is provided but
            (cJSON_IsString(sta_bssid) && !( // if its a string
                (strlen(sta_bssid->valuestring) == 17) || // it must be a valid MAC address length or
                (strlen(sta_bssid->valuestring) == 0)     // an empty string
            )) ||
            (!cJSON_IsString(sta_bssid) && !cJSON_IsNull(sta_bssid)) // if not a string, it must be null
        ))
    {
        ESP_LOGW(TAG, "Wifi Config error: bssid must be a valid mac address or an empty string or null");
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "status", "bssid must be a valid mac address or an empty string or null");
        return return_json_object(root);
    }
    if (sta_use_specific_bssid != NULL && !cJSON_IsBool(sta_use_specific_bssid))
    {
        ESP_LOGW(TAG, "Wifi Config error: use_specific_bssid must be a boolean");
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "status", "use_specific_bssid must be a boolean");
        return return_json_object(root);
    }

    ESP_LOGI(TAG, "Received WiFi config: SSID=%s, Password=%s, BSSID=%s, use_specific_bssid=%s, Hostname=%s",
             sta_ssid ? sta_ssid->valuestring : "N/A", 
             sta_password ? sta_password->valuestring : "N/A",
             (sta_bssid != NULL && !cJSON_IsNull(sta_bssid)) ? sta_bssid->valuestring : "N/A",
             sta_use_specific_bssid != NULL ? (sta_use_specific_bssid->valueint ? "true" : "false") : "N/A",
             sta_hostname ? sta_hostname->valuestring : "N/A");

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
    if (sta_ssid != NULL){
        ESP_LOGI(TAG, "Setting STA SSID to: %s", sta_ssid->valuestring);
        nvs_set_str(nvs_handle, NVS_KEY_STA_SSID, sta_ssid->valuestring);
    }

    if (sta_password != NULL){
        ESP_LOGI(TAG, "Setting STA Password to: %s", sta_password->valuestring);
        nvs_set_str(nvs_handle, NVS_KEY_STA_PASSWORD, sta_password->valuestring);
    }

    if (sta_bssid != NULL){
        ESP_LOGI(TAG, "Setting STA BSSID to: %s", sta_bssid->valuestring ? sta_bssid->valuestring : DEFAULT_WIFI_STA_BSSID);
        nvs_set_str(nvs_handle, NVS_KEY_STA_BSSID, sta_bssid->valuestring ? sta_bssid->valuestring : DEFAULT_WIFI_STA_BSSID);
    }

    if (sta_use_specific_bssid != NULL){
        ESP_LOGI(TAG, "Setting STA use_specific_bssid to: %s", sta_use_specific_bssid->valueint ? "true" : "false");
        nvs_set_i8(nvs_handle, NVS_KEY_STA_USE_SPECIFIC_BSSID, sta_use_specific_bssid->valueint);
    }

    if (sta_hostname != NULL){
        ESP_LOGI(TAG, "Setting STA Hostname to: %s", sta_hostname->valuestring);
        nvs_set_str(nvs_handle, NVS_KEY_STA_HOSTNAME, sta_hostname->valuestring);
    }
    
    // Commit the changes
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) committing NVS changes!", esp_err_to_name(err));
    }
    nvs_close(nvs_handle);
    cJSON_AddBoolToObject(root, "success", true);
    return return_json_object(root);
}
#include "http_server.h"

static const char *TAG = "http_api_wifi_scan.c";


/* Start WiFi scan
 * Endpoint: /api/wifi/scan/start
 *
 * Example JSON response:
 * {
 *   "success": true,
 *   "status": "scan_started" or "scan_already_in_progress" or "scan_start_failed"
 * }
 */
char *get_wifi_scan_start(httpd_req_t *req)
{
    // Start WiFi scan logic here
    // For demonstration, we will just return a success message
    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    bool scanning = xEventGroupGetBits(((http_server_ctx_t *)req->user_ctx)->wifi_event_group) & SCANNING_BIT;
    if (scanning)
    {
        cJSON_AddBoolToObject(root, "success", true);
        cJSON_AddStringToObject(root, "status", "scan_already_in_progress");
        return return_json_object(root);
    }

    esp_err_t err = esp_wifi_scan_start(NULL, false);
    if (err == ESP_OK)
    {
        xEventGroupSetBits(((http_server_ctx_t *)req->user_ctx)->wifi_event_group, SCANNING_BIT);

        cJSON_AddBoolToObject(root, "success", true);
        cJSON_AddStringToObject(root, "status", "scan_started");
        return return_json_object(root);
    }
    else
    {
        ESP_LOGW(TAG, "Failed to start WiFi scan: %s (%d)", esp_err_to_name(err), err);
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "status", "scan_start_failed");
        return return_json_object(root);
    }
}

/* Get WiFi scan results
 * Endpoint: /api/wifi/scan/results
 *
 * Example JSON response when scanning:
 * {
 *   "success": true,
 *   "scanning": true,
 *   "scan_done": false,
 *   "status": "scan_in_progress"
 * }
 *
 * Example JSON response when scan is done:
 * {
 *   "success": true,
 *   "scanning": false,
 *   "scan_done": true,
 *   "status": "scan_completed",
 *   "networks": [
 *     {
 *       "ssid": "Network1",
 *       "bssid": "00:11:22:33:44:55",
 *       "rssi": -45,
 *       "channel": 6,
 *       "authmode": 3
 *     },
 *     ...
 *   ]
 * }
 *
 * Example JSON response when no scan has been initiated:
 * {
 *   "success": true,
 *   "scanning": false,
 *   "scan_done": false,
 *   "status": "no_scan_results_available"
 * }
 */
char *get_wifi_scan_results(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    bool scanning = xEventGroupGetBits(((http_server_ctx_t *)req->user_ctx)->wifi_event_group) & SCANNING_BIT;
    bool scan_done = xEventGroupGetBits(((http_server_ctx_t *)req->user_ctx)->wifi_event_group) & SCAN_DONE_BIT;

    if (scanning)
    {
        cJSON_AddBoolToObject(root, "success", true);
        cJSON_AddBoolToObject(root, "scanning", true);
        cJSON_AddBoolToObject(root, "scan_done", false);
        cJSON_AddStringToObject(root, "status", "scan_in_progress");
        return return_json_object(root);
    }
    else if (scan_done)
    {
        cJSON_AddBoolToObject(root, "success", true);
        cJSON_AddBoolToObject(root, "scanning", false);
        cJSON_AddBoolToObject(root, "scan_done", true);
        uint16_t ap_count = 0;
        esp_wifi_scan_get_ap_num(&ap_count);
        wifi_ap_record_t *ap_records = malloc(sizeof(wifi_ap_record_t) * ap_count);
        if (ap_records == NULL)
        {
            cJSON_AddStringToObject(root, "status", "memory_allocation_failed");
            return return_json_object(root);
        }

        esp_wifi_scan_get_ap_records(&ap_count, ap_records);

        ESP_LOGI(TAG, "WiFi scan found %d access points", ap_count);

        cJSON *ap_array = cJSON_CreateArray();
        char bssid[18];
        for (int i = 0; i < ap_count; i++)
        {
            // Skip networks with empty SSID (hidden networks)
            if (strlen((char *)ap_records[i].ssid) == 0)
            {
                ESP_LOGI(TAG, "Skipping hidden network with empty SSID");
                continue;
            }
            cJSON *ap_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(ap_obj, "ssid", (const char *)ap_records[i].ssid);
            sprintf(bssid, MACSTR, MAC2STR(ap_records[i].bssid));

            cJSON_AddStringToObject(ap_obj, "bssid", bssid);
            cJSON_AddNumberToObject(ap_obj, "rssi", ap_records[i].rssi);
            cJSON_AddNumberToObject(ap_obj, "channel", ap_records[i].primary);
            cJSON_AddNumberToObject(ap_obj, "authmode", ap_records[i].authmode);
            cJSON_AddItemToArray(ap_array, ap_obj);

            ESP_LOGI(TAG, "SSID: %20s, BSSID: %s, RSSI: %4d, Channel: %2d, Authmode: %d",
                     ap_records[i].ssid, bssid, ap_records[i].rssi,
                     ap_records[i].primary, ap_records[i].authmode);
        }
        free(ap_records);

        cJSON_AddStringToObject(root, "status", "scan_completed");
        cJSON_AddItemToObject(root, "networks", ap_array);

        // Clear the SCANNING_BIT and SCAN_DONE_BIT
        xEventGroupClearBits(((http_server_ctx_t *)req->user_ctx)->wifi_event_group, SCANNING_BIT | SCAN_DONE_BIT);

        return return_json_object(root);
    }
    else
    {
        cJSON_AddBoolToObject(root, "success", true);
        cJSON_AddBoolToObject(root, "scanning", false);
        cJSON_AddBoolToObject(root, "scan_done", false);
        cJSON_AddStringToObject(root, "status", "no_scan_results_available");
        return return_json_object(root);
    }
}


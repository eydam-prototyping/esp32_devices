#include "http_server.h"

static const char *TAG = "http_api_wifi_status.c";

/* Get WiFi status as JSON
 * Endpoint: /api/wifi/status
 *
 * Example JSON response:
 * {
 *   "ap_running": true,
 *   "ap_ssid": "ESP32_AP",
 *   "ap_password": "esp32testpassword",
 *   "ap_ip": "192.168.4.1",
 *   "sta_connected": true,
 *   "sta_ssid": "Your_SSID",
 *   "sta_password": "Your_Password",
 *   "sta_has_ip": true,
 *   "sta_ip": "192.168.1.100",
 *   "sta_mac": "00:11:22:33:44:55",
 *   "sta_gateway": "192.168.1.1",
 *   "sta_netmask": "255.255.255.0",
 *   "sta_dns": "8.8.8.8"
 * }
 */
char *get_wifi_status_json(httpd_req_t *req)
{
    // Create a JSON object
    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    bool ap_running = xEventGroupGetBits(((http_server_ctx_t *)req->user_ctx)->wifi_event_group) & AP_RUNNING_BIT;
    bool sta_connected = xEventGroupGetBits(((http_server_ctx_t *)req->user_ctx)->wifi_event_group) & STA_CONNECTED_BIT;
    bool sta_has_ip = xEventGroupGetBits(((http_server_ctx_t *)req->user_ctx)->wifi_event_group) & STA_HAS_IP_BIT;

    cJSON_AddBoolToObject(root, "ap_running", ap_running);
    if (ap_running) {
            
        wifi_config_t wifi_ap_config;
        esp_wifi_get_config(WIFI_IF_AP, &wifi_ap_config);
        cJSON_AddStringToObject(root, "ap_ssid", (const char *)wifi_ap_config.ap.ssid);
        cJSON_AddStringToObject(root, "ap_password", (const char *)wifi_ap_config.ap.password);
        
        // Get AP IP address
        esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
        if (ap_netif) {
            esp_netif_ip_info_t ap_ip_info;
            if (esp_netif_get_ip_info(ap_netif, &ap_ip_info) == ESP_OK) {
                char ap_ip_str[16];
                snprintf(ap_ip_str, sizeof(ap_ip_str), IPSTR, IP2STR(&ap_ip_info.ip));
                cJSON_AddStringToObject(root, "ap_ip", ap_ip_str);
            } else {
                cJSON_AddStringToObject(root, "ap_ip", "192.168.4.1");  // Default fallback
            }
        } else {
            cJSON_AddStringToObject(root, "ap_ip", "192.168.4.1");  // Default fallback
        }
    }
    cJSON_AddBoolToObject(root, "sta_connected", sta_connected);
    if (sta_connected) {
        
        wifi_config_t wifi_sta_config;
        esp_wifi_get_config(WIFI_IF_STA, &wifi_sta_config);
        cJSON_AddStringToObject(root, "sta_ssid", (const char *)wifi_sta_config.sta.ssid);
        cJSON_AddStringToObject(root, "sta_password", (const char *)wifi_sta_config.sta.password);
        cJSON_AddBoolToObject(root, "sta_has_ip", sta_has_ip);
        int rssi;
        esp_err_t err = esp_wifi_sta_get_rssi(&rssi);
        if (err == ESP_OK) {
            cJSON_AddNumberToObject(root, "rssi", rssi);
        } else {
            cJSON_AddNumberToObject(root, "rssi", 0);
        }

        if (sta_has_ip) {
            // Get STA IP information
            esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (sta_netif) {
                esp_netif_ip_info_t sta_ip_info;
                if (esp_netif_get_ip_info(sta_netif, &sta_ip_info) == ESP_OK) {
                    char ip_str[16], gw_str[16], netmask_str[16];
                    snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&sta_ip_info.ip));
                    snprintf(gw_str, sizeof(gw_str), IPSTR, IP2STR(&sta_ip_info.gw));
                    snprintf(netmask_str, sizeof(netmask_str), IPSTR, IP2STR(&sta_ip_info.netmask));
                    
                    cJSON_AddStringToObject(root, "sta_ip", ip_str);
                    cJSON_AddStringToObject(root, "sta_gateway", gw_str);
                    cJSON_AddStringToObject(root, "sta_netmask", netmask_str);
                } else {
                    cJSON_AddStringToObject(root, "sta_ip", "0.0.0.0");
                    cJSON_AddStringToObject(root, "sta_gateway", "0.0.0.0");
                    cJSON_AddStringToObject(root, "sta_netmask", "0.0.0.0");
                }
                
                // Get MAC address
                uint8_t sta_mac[6];
                if (esp_netif_get_mac(sta_netif, sta_mac) == ESP_OK) {
                    char mac_str[18];
                    snprintf(mac_str, sizeof(mac_str), MACSTR, MAC2STR(sta_mac));
                    cJSON_AddStringToObject(root, "sta_mac", mac_str);
                } else {
                    cJSON_AddStringToObject(root, "sta_mac", "00:00:00:00:00:00");
                }
                
                // Get DNS servers
                esp_netif_dns_info_t dns_info;
                if (esp_netif_get_dns_info(sta_netif, ESP_NETIF_DNS_MAIN, &dns_info) == ESP_OK) {
                    char dns_str[16];
                    snprintf(dns_str, sizeof(dns_str), IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
                    cJSON_AddStringToObject(root, "sta_dns", dns_str);
                } else {
                    cJSON_AddStringToObject(root, "sta_dns", "0.0.0.0");
                }
            } else {
                // Fallback if netif not found
                cJSON_AddStringToObject(root, "sta_ip", "0.0.0.0");
                cJSON_AddStringToObject(root, "sta_mac", "00:00:00:00:00:00");
                cJSON_AddStringToObject(root, "sta_gateway", "0.0.0.0");
                cJSON_AddStringToObject(root, "sta_netmask", "0.0.0.0");
                cJSON_AddStringToObject(root, "sta_dns", "0.0.0.0");
            }
        }
    }

    return return_json_object(root);
}
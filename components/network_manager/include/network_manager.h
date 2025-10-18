#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif_net_stack.h"
#include "esp_netif.h"
#include "esp_log.h"

void load_wifi_config(void);
void run_network_manager(void);
EventGroupHandle_t get_wifi_event_group(void);

#define DEFAULT_WIFI_STA_SSID                   ""
#define DEFAULT_WIFI_STA_PASSWORD               ""
#define DEFAULT_WIFI_STA_BSSID                  ""
#define DEFAULT_WIFI_STA_USE_SPECIFIC_BSSID     false
#define DEFAULT_WIFI_STA_HOSTNAME               "ESP32_DEVICE"
#define DEFAULT_WIFI_AP_SSID                    "ESP32_AP"
#define DEFAULT_WIFI_AP_PASSWORD                "esp32testpassword"
#define DEFAULT_WIFI_AP_CHANNEL                 1
#define DEFAULT_WIFI_AP_MAX_CONNECTIONS         5
#define DEFAULT_WIFI_AP_HOSTNAME                "ESP32_DEVICE"


#define AP_RUNNING_BIT          BIT0    // AP is running
#define STA_CONNECTED_BIT       BIT1    // STA connected to AP
#define STA_FAILED              BIT2    // STA failed to connect after maximum retries
#define STA_HAS_IP_BIT          BIT3    // STA has IP address
#define ETH_CONNECTED_BIT       BIT4    // Ethernet connected
#define ETH_HAS_IP_BIT          BIT5    // Ethernet has IP address
#define SCANNING_BIT            BIT6    // WiFi scan in progress
#define SCAN_DONE_BIT           BIT7    // WiFi scan done


#define NVS_NAMESPACE_WIFI_CONFIG       "wifi_config"
#define NVS_KEY_STA_SSID                "sta_ssid"
#define NVS_KEY_STA_PASSWORD            "sta_passwd"
#define NVS_KEY_STA_BSSID               "sta_bssid"
#define NVS_KEY_STA_USE_SPECIFIC_BSSID  "sta_usebss"
#define NVS_KEY_STA_HOSTNAME            "sta_hostna"
#define NVS_KEY_AP_SSID                 "ap_ssid"
#define NVS_KEY_AP_PASSWORD             "ap_passwd"
#define NVS_KEY_AP_CHANNEL              "ap_channel"
#define NVS_KEY_AP_MAX_CONNECTIONS      "ap_max_conn"
#define NVS_KEY_AP_HOSTNAME             "ap_hostna"

extern char STA_SSID[32];
extern char STA_PASSWORD[64];
extern char STA_BSSID[18];
extern bool STA_USE_SPECIFIC_BSSID;
extern char STA_HOSTNAME[32];

extern char AP_SSID[32];
extern char AP_PASSWORD[64];
extern uint8_t AP_CHANNEL;
extern uint8_t AP_MAX_CONNECTIONS;
extern char AP_HOSTNAME[32];

#define MAXIMUM_WIFI_RETRY_COUNT  5
extern uint8_t wifi_retry_count;

#endif // NETWORK_MANAGER_H
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif_net_stack.h"
#include "esp_netif.h"
#include "esp_log.h"

void run_network_manager(void);
EventGroupHandle_t get_wifi_event_group(void);

#define AP_RUNNING_BIT          BIT0    // AP is running
#define STA_CONNECTED_BIT       BIT1    // STA connected to AP
#define STA_FAILED              BIT2    // STA failed to connect after maximum retries
#define STA_HAS_IP_BIT          BIT3    // STA has IP address
#define ETH_CONNECTED_BIT       BIT4    // Ethernet connected
#define ETH_HAS_IP_BIT          BIT5    // Ethernet has IP address
#define SCANNING_BIT            BIT6    // WiFi scan in progress
#define SCAN_DONE_BIT           BIT7    // WiFi scan done


#define NVS_NAMESPACE_WIFI_CONFIG "wifi_config"
#define NVS_KEY_STA_SSID "sta_ssid"
#define NVS_KEY_STA_PASSWORD "sta_password"
#define NVS_KEY_STA_BSSID "sta_bssid"
#define NVS_KEY_STA_USE_SPECIFIC_BSSID "sta_use_specific_bssid"
#define NVS_KEY_AP_SSID "ap_ssid"
#define NVS_KEY_AP_PASSWORD "ap_password"
#define NVS_KEY_AP_CHANNEL "ap_channel"
#define NVS_KEY_AP_MAX_CONNECTIONS "ap_max_connections"
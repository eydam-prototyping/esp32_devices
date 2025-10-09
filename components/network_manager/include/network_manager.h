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

#include "network_manager.h"

static const char *TAG = "network_manager";
static EventGroupHandle_t s_wifi_event_group;

uint8_t wifi_retry_count = 0;
esp_netif_t *esp_netif_sta = NULL;
esp_netif_t *esp_netif_ap = NULL;

static void network_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);

void run_network_manager(void);

void wifi_init_softap(void)
{
    esp_netif_ap = esp_netif_create_default_wifi_ap();

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid_len = strlen(AP_SSID),
            .channel = AP_CHANNEL,
            .max_connection = AP_MAX_CONNECTIONS,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };
    strcpy((char*)wifi_ap_config.ap.ssid, AP_SSID);
    strcpy((char*)wifi_ap_config.ap.password, AP_PASSWORD);

    if (strlen(AP_PASSWORD) == 0) {
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    esp_netif_set_hostname(esp_netif_ap, AP_HOSTNAME);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             AP_SSID, AP_PASSWORD, AP_CHANNEL);
}

void wifi_init_sta(void){
    esp_netif_sta = esp_netif_create_default_wifi_sta();

    wifi_config_t wifi_sta_config = {
        .sta = {
            .scan_method = WIFI_FAST_SCAN,
            .bssid_set = STA_USE_SPECIFIC_BSSID,
        }
    };

    strcpy((char*)wifi_sta_config.sta.ssid, STA_SSID);
    strcpy((char*)wifi_sta_config.sta.password, STA_PASSWORD);
    if (STA_USE_SPECIFIC_BSSID) {
        sscanf(STA_BSSID, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &wifi_sta_config.sta.bssid[0], &wifi_sta_config.sta.bssid[1], &wifi_sta_config.sta.bssid[2],
               &wifi_sta_config.sta.bssid[3], &wifi_sta_config.sta.bssid[4], &wifi_sta_config.sta.bssid[5]);
    }

    esp_netif_set_hostname(esp_netif_sta, STA_HOSTNAME);


    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));

    ESP_LOGI(TAG, "wifi_init_sta finished. SSID:%s password:%s",
             STA_SSID, (strlen(STA_PASSWORD) == 0) ? "<empty>" : "********");
}

void run_network_manager(void){
    ESP_LOGI(TAG, "Starting network manager...");
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Register event handler for Wi-Fi and IP related events
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &network_event_handler,
                                                        NULL,
                                                        NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &network_event_handler,
                                                        NULL,
                                                        NULL));
    

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    load_wifi_config();

    if (strcmp(STA_SSID, "") == 0) {
        ESP_LOGI(TAG, "Starting in APSTA mode, because SSID is empty.");
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        wifi_init_softap();
    } else {
        ESP_LOGI(TAG, "Starting in STA mode.");
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        wifi_init_sta();
    }

    ESP_ERROR_CHECK(esp_wifi_start());
}

EventGroupHandle_t get_wifi_event_group(void)
{
    return s_wifi_event_group;
}

static void network_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_WIFI_READY:
            ESP_LOGD(TAG, "WIFI_EVENT_WIFI_READY");
            {
            }
            break;
        case WIFI_EVENT_SCAN_DONE:
            ESP_LOGD(TAG, "WIFI_EVENT_SCAN_DONE");
            {
                xEventGroupClearBits(s_wifi_event_group, SCANNING_BIT);
                xEventGroupSetBits(s_wifi_event_group, SCAN_DONE_BIT);
            }
            break;
        case WIFI_EVENT_STA_START:
            ESP_LOGD(TAG, "WIFI_EVENT_STA_START");
            {
                esp_wifi_connect();
                ESP_LOGI(TAG, "Station started");
            }
            break;
        case WIFI_EVENT_STA_STOP:
            ESP_LOGD(TAG, "WIFI_EVENT_STA_STOP");
            {
            }
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGD(TAG, "WIFI_EVENT_STA_CONNECTED");
            {
                xEventGroupSetBits(s_wifi_event_group, STA_CONNECTED_BIT);
                wifi_retry_count = 0;
                ESP_LOGI(TAG, "Connected to AP");
            }
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGD(TAG, "WIFI_EVENT_STA_DISCONNECTED");
            {
                xEventGroupClearBits(s_wifi_event_group, STA_CONNECTED_BIT);
                xEventGroupClearBits(s_wifi_event_group, STA_HAS_IP_BIT);
                if (wifi_retry_count < MAXIMUM_WIFI_RETRY_COUNT) {
                    esp_wifi_connect();
                    wifi_retry_count++;
                    ESP_LOGI(TAG, "Retrying to connect to the AP (Attempt %d of %d)", wifi_retry_count, MAXIMUM_WIFI_RETRY_COUNT);
                } else {
                    xEventGroupSetBits(s_wifi_event_group, STA_FAILED);
                    ESP_LOGI(TAG, "Failed to connect to the AP after %d attempts", MAXIMUM_WIFI_RETRY_COUNT);
                    if (strcmp(AP_SSID, "") != 0) {
                        ESP_LOGI(TAG, "Starting APSTA mode");
                        esp_wifi_set_mode(WIFI_MODE_APSTA);
                        wifi_init_softap();
                        esp_wifi_start();
                    }
                }
            }
            break;
        case WIFI_EVENT_STA_AUTHMODE_CHANGE:
            ESP_LOGD(TAG, "WIFI_EVENT_STA_AUTHMODE_CHANGE");
            {
            }
            break;
        case WIFI_EVENT_AP_START:
            ESP_LOGD(TAG, "WIFI_EVENT_AP_START");
            {
                xEventGroupSetBits(s_wifi_event_group, AP_RUNNING_BIT);
            }
            break;
        case WIFI_EVENT_AP_STOP:
            ESP_LOGD(TAG, "WIFI_EVENT_AP_STOP");
            {
                xEventGroupClearBits(s_wifi_event_group, AP_RUNNING_BIT);
            }
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            ESP_LOGD(TAG, "WIFI_EVENT_AP_STACONNECTED");
            {
                wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
                ESP_LOGI(TAG, "Station " MACSTR " joined, AID=%d",
                         MAC2STR(event->mac), event->aid);
            }
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            ESP_LOGD(TAG, "WIFI_EVENT_AP_STADISCONNECTED");
            {
                wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
                ESP_LOGI(TAG, "Station " MACSTR " left, AID=%d, reason:%d",
                         MAC2STR(event->mac), event->aid, event->reason);
            }
            break;
        case WIFI_EVENT_AP_PROBEREQRECVED:
            ESP_LOGD(TAG, "WIFI_EVENT_AP_PROBEREQRECVED");
            {
            }
            break;
        case WIFI_EVENT_AP_WRONG_PASSWORD:
            ESP_LOGD(TAG, "WIFI_EVENT_AP_WRONG_PASSWORD");
            {
            }
            break;
        case WIFI_EVENT_MAX:
            ESP_LOGD(TAG, "WIFI_EVENT_MAX");
            {
                ESP_LOGW(TAG, "WIFI_EVENT_MAX received, don't know what to do...");
            }
            break;
        default:
            ESP_LOGD(TAG, "Unhandled WIFI event: %d", event_id);
            break;
        }
    }
    else if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:
            ESP_LOGD(TAG, "IP_EVENT_STA_GOT_IP");
            {
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
                xEventGroupSetBits(s_wifi_event_group, STA_HAS_IP_BIT);
            }
            break;
        case IP_EVENT_STA_LOST_IP:
            ESP_LOGD(TAG, "IP_EVENT_STA_LOST_IP");
            {
                xEventGroupClearBits(s_wifi_event_group, STA_HAS_IP_BIT);
            }
            break;
        case IP_EVENT_AP_STAIPASSIGNED:
            ESP_LOGD(TAG, "IP_EVENT_AP_STAIPASSIGNED");
            {
            }
            break;
        case IP_EVENT_GOT_IP6:
            ESP_LOGD(TAG, "IP_EVENT_GOT_IP6");
            {
            }
            break;
        case IP_EVENT_ETH_GOT_IP:
            ESP_LOGD(TAG, "IP_EVENT_ETH_GOT_IP");
            {
            }
            break;
        case IP_EVENT_ETH_LOST_IP:
            ESP_LOGD(TAG, "IP_EVENT_ETH_LOST_IP");
            {
            }
            break;
        case IP_EVENT_PPP_GOT_IP:
            ESP_LOGD(TAG, "IP_EVENT_PPP_GOT_IP");
            {
            }
            break;
        case IP_EVENT_PPP_LOST_IP:
            ESP_LOGD(TAG, "IP_EVENT_PPP_LOST_IP");
            {
            }
            break;
        default:
            break;
        }
    }
}
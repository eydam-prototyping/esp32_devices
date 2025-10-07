#include "network_manager.h"

static const char *TAG = "network_manager";

#define EXAMPLE_ESP_WIFI_AP_SSID            "ESP32_AP"
#define EXAMPLE_ESP_WIFI_AP_PASSWD          "esp32testpassword"
#define EXAMPLE_ESP_WIFI_CHANNEL            1
#define EXAMPLE_MAX_STA_CONN                5

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);

void run_network_manager(void);

esp_netif_t *wifi_init_softap(void)
{
    esp_netif_t *esp_netif_ap = esp_netif_create_default_wifi_ap();

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_AP_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_AP_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_AP_PASSWD,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    if (strlen(EXAMPLE_ESP_WIFI_AP_PASSWD) == 0) {
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_AP_SSID, EXAMPLE_ESP_WIFI_AP_PASSWD, EXAMPLE_ESP_WIFI_CHANNEL);

    return esp_netif_ap;
}

void run_network_manager(void){
    ESP_LOGI(TAG, "Starting network manager...");
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
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    esp_netif_t *esp_netif_ap = wifi_init_softap();

    ESP_ERROR_CHECK(esp_wifi_start());
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
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
            }
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGD(TAG, "WIFI_EVENT_STA_DISCONNECTED");
            {
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
            }
            break;
        case WIFI_EVENT_AP_STOP:
            ESP_LOGD(TAG, "WIFI_EVENT_AP_STOP");
            {
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
            }
            break;
        case IP_EVENT_STA_LOST_IP:
            ESP_LOGD(TAG, "IP_EVENT_STA_LOST_IP");
            {
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
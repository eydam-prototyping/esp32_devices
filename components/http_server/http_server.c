#include "http_server.h"

static const char *TAG = "http_server";

void http_server_task(void *pvParameters)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    EventGroupHandle_t wifi_event_group = (EventGroupHandle_t)pvParameters;
    // Wait until either the AP or STA bits are set in the event group.
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           AP_RUNNING_BIT | STA_HAS_IP_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

    // Start the httpd server
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Register URI handlers
        register_uri_handlers(server);
    }
    for (int i = 300; i >= 0; i--)
    {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

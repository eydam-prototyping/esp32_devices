#include "http_server.h"

static const char *TAG = "http_server";

void run_http_server(http_server_ctx_t *ctx){
    ESP_LOGI(TAG, "Initializing SPIFFS...");
    if (spiffs_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize SPIFFS");
        return;
    }
    ESP_LOGI(TAG, "SPIFFS initialized successfully");

    ESP_LOGI(TAG, "Initializing HTTP server...");
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.uri_match_fn = httpd_uri_match_wildcard;
    ESP_LOGI(TAG, "Starting HTTP server...");

    // Wait until either the AP or STA bits are set in the event group.
    EventBits_t bits = xEventGroupWaitBits(ctx->wifi_event_group,
                                           AP_RUNNING_BIT | STA_HAS_IP_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    ESP_LOGI(TAG, "Started server on port: %d", config.server_port);

    // Start the httpd server
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Register URI handlers
        register_uri_handlers(server, ctx);
    }
}
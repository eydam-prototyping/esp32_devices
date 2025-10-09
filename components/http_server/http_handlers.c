#include "http_server.h"

static const char *TAG = "http_server";

static esp_err_t spiffs_get_handler(httpd_req_t *req)
{
    const char* uri = req->uri;
    ESP_LOGI(TAG, "Serving file from SPIFFS: %s", uri);

    const char resp[] = "Hello World from SPIFFS!";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_uri_t spiffs_get = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = spiffs_get_handler,
};

esp_err_t register_uri_handlers(httpd_handle_t server)
{
    return httpd_register_uri_handler(server, &spiffs_get);
}
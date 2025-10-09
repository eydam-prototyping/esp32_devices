#include "http_server.h"

static const char *TAG = "http_server";

static esp_err_t spiffs_get_handler(httpd_req_t *req)
{
    const char* uri = req->uri;
    ESP_LOGI(TAG, "Serving file from SPIFFS: %s", uri);

    if (strcmp(uri, "/") == 0) {
        uri = "/index.html";
    }
    
    if (!spiffs_file_exists(uri)) {
        ESP_LOGW(TAG, "File not found in SPIFFS: %s", uri);
        httpd_resp_send_404(req);
        return ESP_ERR_NOT_FOUND;
    }
    
    return spiffs_serve_file(req, uri);
}

httpd_uri_t spiffs_get_default = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = spiffs_get_handler,
};

httpd_uri_t spiffs_get_static = {
    .uri       = "/*",
    .method    = HTTP_GET,
    .handler   = spiffs_get_handler,
};


static esp_err_t api_handler(httpd_req_t *req)
{
    const char* uri = req->uri;
    ESP_LOGI(TAG, "API request: %s", uri);

    httpd_resp_send_404(req);
    return ESP_ERR_NOT_FOUND;
}

httpd_uri_t api_get = {
    .uri       = "/api/*",
    .method    = HTTP_GET,
    .handler   = api_handler,
};

esp_err_t register_uri_handlers(httpd_handle_t server)
{
    esp_err_t ret = ESP_OK;
    
    ret |= httpd_register_uri_handler(server, &api_get);    // Register API handler first
    ret |= httpd_register_uri_handler(server, &spiffs_get_default);
    ret |= httpd_register_uri_handler(server, &spiffs_get_static);
    
    ESP_LOGI(TAG, "Registered URI handlers with result: %s", esp_err_to_name(ret));
    return ret;
}
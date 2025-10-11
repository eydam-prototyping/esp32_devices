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


static esp_err_t api_get_handler(httpd_req_t *req)
{
    const char* uri = req->uri;
    ESP_LOGI(TAG, "API request: %s", uri);

    if (strcmp(uri, "/api/wifi/status") == 0) {
        const char* resp_str = get_wifi_status_json(req);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp_str, strlen(resp_str));
        return ESP_OK;
    }

    if (strcmp(uri, "/api/wifi/scan/start") == 0) {
        const char* resp_str = get_wifi_scan_start(req);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp_str, strlen(resp_str));
        return ESP_OK;
    }

    if (strcmp(uri, "/api/wifi/scan/results") == 0) {
        const char* resp_str = get_wifi_scan_results(req);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp_str, strlen(resp_str));
        return ESP_OK;
    }

    if (strcmp(uri, "/api/wifi/config/ap") == 0) {
        const char* resp_str = get_wifi_config_ap(req);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp_str, strlen(resp_str));
        return ESP_OK;
    }

    if (strcmp(uri, "/api/wifi/config/sta") == 0) {
        const char* resp_str = get_wifi_config_sta(req);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp_str, strlen(resp_str));
        return ESP_OK;
    }

    httpd_resp_send_404(req);
    return ESP_ERR_NOT_FOUND;
}

httpd_uri_t api_get = {
    .uri       = "/api/*",
    .method    = HTTP_GET,
    .handler   = api_get_handler,
};

static esp_err_t api_post_handler(httpd_req_t *req)
{
    const char* uri = req->uri;
    ESP_LOGI(TAG, "API request: %s", uri);

    if (strcmp(uri, "/api/wifi/config/ap") == 0) {
        const char* resp_str = post_wifi_config_ap(req);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp_str, strlen(resp_str));
        return ESP_OK;
    }

    if (strcmp(uri, "/api/wifi/config/sta") == 0) {
        const char* resp_str = post_wifi_config_sta(req);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp_str, strlen(resp_str));
        return ESP_OK;
    }

    httpd_resp_send_404(req);
    return ESP_ERR_NOT_FOUND;
}

httpd_uri_t api_post = {
    .uri       = "/api/*",
    .method    = HTTP_POST,
    .handler   = api_post_handler,
};

esp_err_t register_uri_handlers(httpd_handle_t server, http_server_ctx_t *ctx)
{
    esp_err_t ret = ESP_OK;

    api_get.user_ctx = ctx;
    api_post.user_ctx = ctx;
    
    ret |= httpd_register_uri_handler(server, &api_get);    // Register API handler first
    ret |= httpd_register_uri_handler(server, &api_post); 
    ret |= httpd_register_uri_handler(server, &spiffs_get_default);
    ret |= httpd_register_uri_handler(server, &spiffs_get_static);
    
    ESP_LOGI(TAG, "Registered URI handlers with result: %s", esp_err_to_name(ret));
    return ret;
}
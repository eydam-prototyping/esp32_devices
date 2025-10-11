#include <esp_http_server.h>
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include <sys/stat.h>
#include "network_manager.h"
#include "cJSON.h"


typedef struct http_server_ctx {
    EventGroupHandle_t wifi_event_group;
} http_server_ctx_t;

void run_http_server(http_server_ctx_t *ctx);
esp_err_t register_uri_handlers(httpd_handle_t server, http_server_ctx_t *ctx);

char *return_json_object(cJSON *root);

char *get_wifi_status_json(httpd_req_t *req);
char *get_wifi_scan_start(httpd_req_t *req);
char *get_wifi_scan_results(httpd_req_t *req);
char *get_wifi_config_ap(httpd_req_t *req);
char *post_wifi_config_ap(httpd_req_t *req);
char *get_wifi_config_sta(httpd_req_t *req);
char *post_wifi_config_sta(httpd_req_t *req);

esp_err_t spiffs_init(void);
bool spiffs_file_exists(const char *filepath);
esp_err_t spiffs_serve_file(httpd_req_t *req, const char *filepath);
const char *spiffs_get_mime_type(const char *filepath);
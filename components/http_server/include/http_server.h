#include <esp_http_server.h>
#include "esp_log.h"
#include "esp_spiffs.h"
#include <sys/stat.h>
#include "network_manager.h"

esp_err_t register_uri_handlers(httpd_handle_t server);

void run_http_server(EventGroupHandle_t wifi_event_group);

esp_err_t spiffs_init(void);
bool spiffs_file_exists(const char *filepath);
esp_err_t spiffs_serve_file(httpd_req_t *req, const char *filepath);
const char *spiffs_get_mime_type(const char *filepath);
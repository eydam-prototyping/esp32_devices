#include <esp_http_server.h>
#include "esp_log.h"
#include "network_manager.h"

esp_err_t register_uri_handlers(httpd_handle_t server);

void http_server_task( void * pvParameters );
idf_component_register(
    SRCS "http_api_wifi_config.c" "http_api.c" "http_handlers.c" "http_server.c" "spiffs.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_server network_manager spiffs nvs_flash json
    PRIV_REQUIRES vfs
)
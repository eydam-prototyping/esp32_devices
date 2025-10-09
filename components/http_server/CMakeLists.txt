idf_component_register(
    SRCS "http_handlers.c" "http_server.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_server network_manager
    PRIV_REQUIRES vfs
)
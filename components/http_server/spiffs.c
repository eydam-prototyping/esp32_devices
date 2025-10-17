#include "http_server.h"

static const char *TAG = "spiffs";

/**
 * @brief MIME type mapping structure
 */
typedef struct
{
    const char *extension;
    const char *mime_type;
} mime_type_mapping_t;

/**
 * @brief MIME type mappings for common web files
 */
static const mime_type_mapping_t mime_mappings[] = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".ico", "image/x-icon"},
    {".svg", "image/svg+xml"},
    {".txt", "text/plain"},
    {".xml", "text/xml"},
    {NULL, "application/octet-stream"} // Default
};

esp_err_t spiffs_init(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5, // This sets the maximum number of files that can be open at the same time
        .format_if_mount_failed = true};

    // Initialize SPIFFS
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ESP_OK;
}

const char *spiffs_get_mime_type(const char *filepath)
{
    if (!filepath)
    {
        return mime_mappings[sizeof(mime_mappings) / sizeof(mime_mappings[0]) - 1].mime_type;
    }

    // Find the last dot for extension
    const char *ext = strrchr(filepath, '.');
    if (!ext)
    {
        return mime_mappings[sizeof(mime_mappings) / sizeof(mime_mappings[0]) - 1].mime_type;
    }

    // Search for matching MIME type
    for (int i = 0; mime_mappings[i].extension != NULL; i++)
    {
        if (strcasecmp(ext, mime_mappings[i].extension) == 0)
        {
            return mime_mappings[i].mime_type;
        }
    }

    // Return default MIME type
    return mime_mappings[sizeof(mime_mappings) / sizeof(mime_mappings[0]) - 1].mime_type;
}

bool spiffs_file_exists(const char *filepath)
{
    if (!filepath)
    {
        return false;
    }

    char full_path[64];
    snprintf(full_path, sizeof(full_path), "/spiffs%s", filepath);

    struct stat st;
    return (stat(full_path, &st) == 0);
}

esp_err_t spiffs_serve_file(httpd_req_t *req, const char *filepath)
{
    if (!req || !filepath)
    {
        ESP_LOGE(TAG, "Invalid parameters for serving file");
        return ESP_ERR_INVALID_ARG;
    }

    char full_path[64];
    snprintf(full_path, sizeof(full_path), "/spiffs%s", filepath);

    ESP_LOGI(TAG, "Serving file: %s", full_path);

    FILE *file = fopen(full_path, "r");
    if (!file)
    {
        ESP_LOGW(TAG, "File not found: %s", full_path);
        httpd_resp_send_404(req);
        return ESP_ERR_NOT_FOUND;
    }

    // Set MIME type
    const char *mime_type = spiffs_get_mime_type(filepath);
    httpd_resp_set_type(req, mime_type);

    // Add caching headers for static content
    if (strstr(filepath, ".css") || strstr(filepath, ".js") || strstr(filepath, ".png") ||
        strstr(filepath, ".jpg") || strstr(filepath, ".ico"))
    {
        httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=31536000");
    }

    // Stream file content
    char buffer[512];
    size_t bytes_read;
    esp_err_t ret = ESP_OK;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        if (httpd_resp_send_chunk(req, buffer, bytes_read) != ESP_OK)
        {
            ESP_LOGE(TAG, "Error sending file chunk");
            ret = ESP_FAIL;
            break;
        }
    }

    // End chunked response
    if (ret == ESP_OK)
    {
        httpd_resp_send_chunk(req, NULL, 0);
    }

    fclose(file);
    return ret;
}
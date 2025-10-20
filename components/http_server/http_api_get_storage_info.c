#include "http_server.h"
#include "esp_partition.h"
#include "esp_app_format.h"
#include "esp_ota_ops.h"
#include "esp_flash.h"

static const char *TAG = "http_api_get_storage_info.c";

char *get_device_storage_info(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        ESP_LOGW(TAG, "Failed to create root JSON object");
    }

    // Flash Size
    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);
    cJSON_AddNumberToObject(root, "flash_size", flash_size);

    // Flash Configuration
    esp_image_header_t image_header;
    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    if (running_partition != NULL)
    {
        // Flash mode and speed from image header
        if (esp_partition_read(running_partition, 0, &image_header, sizeof(image_header)) == ESP_OK)
        {
            const char *flash_mode = "unknown";
            switch (image_header.spi_mode)
            {
            case ESP_IMAGE_SPI_MODE_QIO:
                flash_mode = "QIO";
                break;
            case ESP_IMAGE_SPI_MODE_QOUT:
                flash_mode = "QOUT";
                break;
            case ESP_IMAGE_SPI_MODE_DIO:
                flash_mode = "DIO";
                break;
            case ESP_IMAGE_SPI_MODE_DOUT:
                flash_mode = "DOUT";
                break;
            default:
                flash_mode = "unknown";
                break;
            }
            cJSON_AddStringToObject(root, "flash_mode", flash_mode);

            const char *flash_speed = "unknown";
            switch (image_header.spi_speed)
            {
            case ESP_IMAGE_SPI_SPEED_DIV_1:
                flash_speed = "80MHz";
                break;
            case ESP_IMAGE_SPI_SPEED_DIV_2:
                flash_speed = "40MHz";
                break;
            case ESP_IMAGE_SPI_SPEED_DIV_3:
                flash_speed = "26MHz";
                break;
            case ESP_IMAGE_SPI_SPEED_DIV_4:
                flash_speed = "20MHz";
                break;
            default:
                flash_speed = "unknown";
                break;
            }
            cJSON_AddStringToObject(root, "flash_speed", flash_speed);
        }

        cJSON_AddStringToObject(root, "running_partition", running_partition->label);
    }

    // SPIFFS Information (if mounted)
    size_t total_spiffs = 0, used_spiffs = 0;
    esp_err_t ret = esp_spiffs_info(NULL, &total_spiffs, &used_spiffs);
    if (ret == ESP_OK)
    {
        cJSON_AddNumberToObject(root, "spiffs_total", total_spiffs);
        cJSON_AddNumberToObject(root, "spiffs_used", used_spiffs);
    }
    else
    {
        cJSON_AddStringToObject(root, "spiffs_status", "not_mounted");
    }

    // Partition Table Information
    cJSON *partitions_array = cJSON_CreateArray();
    if (partitions_array != NULL)
    {
        esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
        if (it != NULL)
        {
            while (it != NULL)
            {
                const esp_partition_t *partition = esp_partition_get(it);
                if (partition != NULL)
                {
                    cJSON *partition_obj = cJSON_CreateObject();
                    if (partition_obj != NULL)
                    {
                        cJSON_AddStringToObject(partition_obj, "label", partition->label);

                        const char *type_str = "unknown";
                        switch (partition->type)
                        {
                        case ESP_PARTITION_TYPE_APP:
                            type_str = "app";
                            break;
                        case ESP_PARTITION_TYPE_DATA:
                            type_str = "data";
                            break;
                        default:
                            type_str = "unknown";
                            break;
                        }
                        cJSON_AddStringToObject(partition_obj, "type", type_str);
                        cJSON_AddNumberToObject(partition_obj, "size", partition->size);
                        cJSON_AddNumberToObject(partition_obj, "address", partition->address);

                        cJSON_AddItemToArray(partitions_array, partition_obj);
                    }
                    else
                    {
                        ESP_LOGW(TAG, "Failed to create partition object");
                    }
                }
                it = esp_partition_next(it);
            }
            esp_partition_iterator_release(it);
        }
        else
        {
            ESP_LOGW(TAG, "No partitions found");
        }

        cJSON_AddItemToObject(root, "partitions", partitions_array);
    }
    else
    {
        ESP_LOGW(TAG, "Failed to create partitions_array");
    }

    return return_json_object(root);
}
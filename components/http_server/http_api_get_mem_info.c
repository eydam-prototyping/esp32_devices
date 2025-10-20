#include "http_server.h"
#include "esp_heap_caps.h"
#include "esp_flash.h"
#include "esp_timer.h"

static const char *TAG = "http_api_get_meminfo.c";

char *get_device_memory_info(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    if (root != NULL)
    {
        // Uptime
        int64_t uptime_us = esp_timer_get_time();
        uint32_t uptime_seconds = uptime_us / 1000000;
        cJSON_AddNumberToObject(root, "uptime_seconds", uptime_seconds);

        // Heap Memory
        size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
        size_t total_heap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
        size_t largest_free_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);

        cJSON_AddNumberToObject(root, "free_heap", free_heap);
        cJSON_AddNumberToObject(root, "total_heap", total_heap);
        cJSON_AddNumberToObject(root, "largest_free_block", largest_free_block);

        // PSRAM (if available)
        size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        size_t total_psram = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);

        if (total_psram > 0)
        {
            cJSON_AddNumberToObject(root, "free_psram", free_psram);
            cJSON_AddNumberToObject(root, "total_psram", total_psram);
        }
        else
        {
            cJSON_AddStringToObject(root, "psram_status", "not_available");
        }

        #if CONFIG_FREERTOS_USE_TRACE_FACILITY == 1
        // Tasks Information
        UBaseType_t num_tasks = uxTaskGetNumberOfTasks();
        cJSON_AddNumberToObject(root, "num_tasks", num_tasks);
        
        TaskStatus_t *taskStatusArray = pvPortMalloc( num_tasks * sizeof( TaskStatus_t ) );
        if (taskStatusArray != NULL) {
            uxTaskGetSystemState(taskStatusArray, num_tasks, NULL);

            cJSON *tasks_array = cJSON_CreateArray();
            for (int i=0; i < num_tasks; i++)
            {
                cJSON *task_info = cJSON_CreateObject();
                cJSON_AddStringToObject(task_info, "name", taskStatusArray[i].pcTaskName);
                cJSON_AddNumberToObject(task_info, "priority", taskStatusArray[i].uxBasePriority);
                switch (taskStatusArray[i].eCurrentState)
                {
                    case eRunning:
                        cJSON_AddStringToObject(task_info, "state", "running");
                        break;
                    case eReady:
                        cJSON_AddStringToObject(task_info, "state", "ready");
                        break;
                    case eBlocked:
                        cJSON_AddStringToObject(task_info, "state", "blocked");
                        break;
                    case eSuspended:
                        cJSON_AddStringToObject(task_info, "state", "suspended");
                        break;
                    case eDeleted:
                        cJSON_AddStringToObject(task_info, "state", "deleted");
                        break;
                    default:
                        cJSON_AddStringToObject(task_info, "state", "unknown");
                        break;
                }
                cJSON_AddNumberToObject(task_info, "highwatermark", taskStatusArray[i].usStackHighWaterMark);

                cJSON_AddItemToArray(tasks_array, task_info);
            }
            cJSON_AddItemToObject(root, "tasks", tasks_array);
            
            // *** CRITICAL FIX: Free the allocated memory! ***
            vPortFree(taskStatusArray);
        } else {
            ESP_LOGW(TAG, "Failed to allocate memory for taskStatusArray");
        }
        #endif // CONFIG_FREERTOS_USE_TRACE_FACILITY

        return return_json_object(root);
    }
    else
    {
        ESP_LOGW(TAG, "Failed to create memory_info object");
        return NULL;
    }
}

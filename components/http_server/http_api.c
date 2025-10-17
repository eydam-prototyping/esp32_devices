#include "http_server.h"

static const char *TAG = "http_api.c";

char *return_json_object(cJSON *root)
{
    if (root == NULL)
    {
        return NULL;
    }
    char *json_string = cJSON_Print(root);
    cJSON_Delete(root); // Free the JSON object
    return json_string;
}


#include "satellite_server_protocol_parsers.h"

#include <string.h>

#include "esp_log.h"

static const char *TAG = "satellite_server_protocol_parsers";

satellite_message_type_t
satellite_protocol_parse_type(const cJSON *type)
{
    if (!cJSON_IsString(type) || type->valuestring == NULL)
        return SATELLITE_MSG_UNKNOWN;

    if (strcmp(type->valuestring, "click") == 0)
        return SATELLITE_MSG_CLICK;

    if (strcmp(type->valuestring, "imu") == 0)
        return SATELLITE_MSG_IMU;

    return SATELLITE_MSG_UNKNOWN;
}


bool
satellite_protocol_parse_uint64(
    const cJSON *item,
    uint64_t *value)
{
    if (!cJSON_IsNumber(item) || value == NULL)
        return false;

    if (item->valuedouble < 0)
        return false;

    /*
     * cJSON stores numbers as doubles. This is fine for the current
     * protocol while timestamps are within the exact integer range
     * of a double.
     */
    *value = (uint64_t)item->valuedouble;
    return true;
}


bool
satellite_protocol_parse_click(
    const cJSON *root,
    satellite_message_t *message)
{
    const cJSON *time = cJSON_GetObjectItemCaseSensitive(root, "time");

    if (!satellite_protocol_parse_uint64(
            time,
            &message->click.time))
    {
        ESP_LOGW(TAG, "Invalid click time");
        return false;
    }

    return true;
}


bool
satellite_protocol_parse_imu(
    const cJSON *root,
    satellite_message_t *message)
{
    const cJSON *time = cJSON_GetObjectItemCaseSensitive(root, "time");
    const cJSON *x = cJSON_GetObjectItemCaseSensitive(root, "x");
    const cJSON *y = cJSON_GetObjectItemCaseSensitive(root, "y");
    const cJSON *z = cJSON_GetObjectItemCaseSensitive(root, "z");

    if (!satellite_protocol_parse_uint64(
            time,
            &message->imu.time))
    {
        ESP_LOGW(TAG, "Invalid IMU time");
        return false;
    }

    if (!cJSON_IsNumber(x) ||
        !cJSON_IsNumber(y) ||
        !cJSON_IsNumber(z))
    {
        ESP_LOGW(TAG, "Invalid IMU values");
        return false;
    }

    message->imu.x = (float)x->valuedouble;
    message->imu.y = (float)y->valuedouble;
    message->imu.z = (float)z->valuedouble;

    return true;
}

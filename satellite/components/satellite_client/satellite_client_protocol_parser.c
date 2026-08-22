#include "satellite_client_protocol_parser.h"

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "cJSON.h"
#include "esp_err.h"
#include "esp_log.h"

#include "satellite_client_protocol_parser.h"
#include "controller_msg.h"

static const char *TAG = "SATELLITE_CLIENT_PROTOCOL";

/**
 * @brief Parse a request_status message.
 *
 * Expected JSON:
 * {
 *     "type": "request_status"
 * }
 */
esp_err_t satellite_client_protocol_parse_request_status(
    const cJSON *message, satellite_message_tt *out)
{
    if (message == NULL || out == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    out->type = CONTROLLER_CMD_REQUEST_STATUS;

    ESP_LOGI(TAG, "Received status request");

    return ESP_OK;
}

/**
 * @brief Parse a set_time message.
 *
 * Expected JSON:
 * {
 *     "type": "set_time",
 *     "time": 123456789
 * }
 */
static esp_err_t satellite_client_protocol_parse_set_time(
    const cJSON *message,
    satellite_message_tt *out)
{
    if (message == NULL || out == NULL)
        return ESP_ERR_INVALID_ARG;

    const cJSON *time_item = cJSON_GetObjectItemCaseSensitive(
        message,
        "time");

    if (time_item == NULL)
    {
        ESP_LOGW(TAG, "set_time message missing 'time'");
        return ESP_ERR_INVALID_ARG;
    }

    if (!cJSON_IsNumber(time_item))
    {
        ESP_LOGW(TAG, "'time' is not a number");
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * cJSON stores numbers as double, so explicitly validate
     * the range and that the value is an integer before casting.
     */
    if (!isfinite(time_item->valuedouble) ||
        time_item->valuedouble < 0.0 ||
        time_item->valuedouble > UINT32_MAX ||
        floor(time_item->valuedouble) != time_item->valuedouble)
    {
        ESP_LOGW(TAG, "Invalid time value: %f",
                 time_item->valuedouble);

        return ESP_ERR_INVALID_ARG;
    }

    if (message == NULL || out == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Received status request");

    out->type = CONTROLLER_CMD_SET_GAME_TIME;
    out->time = (uint64_t)time_item->valuedouble;

    return ESP_OK;
}

/**
 * @brief Parse a message received from the satellite server.
 */
esp_err_t satellite_client_protocol_parse(
    const cJSON *message, satellite_message_tt *out)
{
    if (message == NULL){return ESP_ERR_INVALID_ARG;}

    const cJSON *type = cJSON_GetObjectItemCaseSensitive(
        message,
        "type");

    if (type == NULL || !cJSON_IsString(type))
    {
        ESP_LOGW(TAG, "Message missing valid 'type'");
        return ESP_ERR_INVALID_ARG;
    }

    if (strcmp(type->valuestring, "heartbeat") == 0)
    {
        return ESP_OK;
    }

    if (strcmp(type->valuestring, "request_status") == 0)
    {
        return satellite_client_protocol_parse_request_status(message, out);
    }

    if (strcmp(type->valuestring, "set_time") == 0)
    {
        esp_err_t err = satellite_client_protocol_parse_set_time(message, out);

        if (err != ESP_OK)
        {
            return err;
        }

        return ESP_OK;
    }

    ESP_LOGW(
        TAG,
        "Unknown message type: %s",
        type->valuestring);

    return ESP_ERR_NOT_FOUND;
}
#include "satellite_client_protocol.h"

#include <string.h>
#include "cJSON.h"
#include "esp_log.h"
#include "satellite_client_espnow.h"

static const char *TAG = "satellite_client_protocol";
static satellite_client_role_t s_role = SATELLITE_CLIENT_ROLE_UNKNOWN;
static bool s_initialized;
satellite_client_protocol_time_callback_t s_time_callback = NULL;

static satellite_client_role_t role_from_json(const cJSON *item)
{
    if (!cJSON_IsString(item) || !item->valuestring)
        return SATELLITE_CLIENT_ROLE_UNKNOWN;
    if (strcmp(item->valuestring, "Left") == 0)
        return SATELLITE_CLIENT_ROLE_LEFT;
    if (strcmp(item->valuestring, "Right") == 0)
        return SATELLITE_CLIENT_ROLE_RIGHT;
    return SATELLITE_CLIENT_ROLE_UNKNOWN;
}

static esp_err_t send_json(
    const uint8_t dest_mac[ESP_NOW_ETH_ALEN], cJSON *root)
{
    char *json = cJSON_PrintUnformatted(root);
    if (!json)
        return ESP_ERR_NO_MEM;

    size_t len = strlen(json);
    if (len > ESP_NOW_MAX_DATA_LEN) {
        cJSON_free(json);
        return ESP_ERR_INVALID_SIZE;
    }
    ESP_LOGI(TAG, "Sending JSON");
    esp_err_t err = satellite_client_espnow_send(
        (const uint8_t *)json, len);
    cJSON_free(json);
    return err;
}

esp_err_t satellite_client_protocol_init(satellite_client_protocol_time_callback_t time_callback)
{
    s_time_callback = time_callback;

    s_role = SATELLITE_CLIENT_ROLE_UNKNOWN;

    return ESP_OK;
}

esp_err_t satellite_client_protocol_request_role(
    const uint8_t server_mac[ESP_NOW_ETH_ALEN])
{
    if (!s_initialized)
        return ESP_ERR_INVALID_STATE;
    if (!server_mac)
        return ESP_ERR_INVALID_ARG;

    cJSON *root = cJSON_CreateObject();
    if (!root)
        return ESP_ERR_NO_MEM;

    cJSON_AddStringToObject(
        root, "type", "request_controller_role");

    esp_err_t err = send_json(server_mac, root);
    cJSON_Delete(root);
    return err;
}

esp_err_t satellite_client_protocol_send_click(
    const uint8_t server_mac[ESP_NOW_ETH_ALEN],
    uint64_t time)
{
    if (!s_initialized || !satellite_client_protocol_has_role())
        return ESP_ERR_INVALID_STATE;
    if (!server_mac)
        return ESP_ERR_INVALID_ARG;

    cJSON *root = cJSON_CreateObject();
    if (!root)
        return ESP_ERR_NO_MEM;

    cJSON_AddStringToObject(root, "type", "click");
    cJSON_AddNumberToObject(root, "time", (double)time);

    esp_err_t err = send_json(server_mac, root);
    cJSON_Delete(root);
    return err;
}

esp_err_t satellite_client_protocol_send_imu(
    const uint8_t server_mac[ESP_NOW_ETH_ALEN],
    uint64_t time,
    float x, float y, float z)
{
    if (!s_initialized || !satellite_client_protocol_has_role())
        return ESP_ERR_INVALID_STATE;
    if (!server_mac)
        return ESP_ERR_INVALID_ARG;

    cJSON *root = cJSON_CreateObject();
    if (!root)
        return ESP_ERR_NO_MEM;

    cJSON_AddStringToObject(root, "type", "imu");
    cJSON_AddNumberToObject(root, "time", (double)time);
    cJSON_AddNumberToObject(root, "x", x);
    cJSON_AddNumberToObject(root, "y", y);
    cJSON_AddNumberToObject(root, "z", z);

    esp_err_t err = send_json(server_mac, root);
    cJSON_Delete(root);
    return err;
}

esp_err_t satellite_client_protocol_handle(const uint8_t src_mac[ESP_NOW_ETH_ALEN], const uint8_t *data, uint16_t data_len)
{
    if (!s_initialized)
        return ESP_ERR_INVALID_STATE;

    if (src_mac == NULL || data == NULL || data_len == 0)
        return ESP_ERR_INVALID_ARG;

    cJSON *root = cJSON_ParseWithLength(
        (const char *)data,
        data_len);

    if (root == NULL || !cJSON_IsObject(root))
    {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    const cJSON *type =
        cJSON_GetObjectItemCaseSensitive(root, "type");

    if (!cJSON_IsString(type) || type->valuestring == NULL)
    {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_ERR_NOT_FOUND;

    /*
     * Controller role assignment
     */
    if (strcmp(
            type->valuestring,
            "reply_controller_role") == 0)
    {
        const cJSON *role =
            cJSON_GetObjectItemCaseSensitive(
                root,
                "role");

        satellite_client_role_t parsed =
            role_from_json(role);

        if (parsed == SATELLITE_CLIENT_ROLE_UNKNOWN)
        {
            ESP_LOGW(
                TAG,
                "Invalid role in controller-role reply");

            err = ESP_ERR_INVALID_ARG;
        }
        else
        {
            s_role = parsed;

            ESP_LOGI(
                TAG,
                "Assigned role: %s",
                parsed == SATELLITE_CLIENT_ROLE_LEFT
                    ? "Left"
                    : "Right");

            err = ESP_OK;
        }
    }

    /*
     * Server time synchronization
     */
    else if (strcmp(
                 type->valuestring,
                 "server_time") == 0)
    {
        const cJSON *time =
            cJSON_GetObjectItemCaseSensitive(
                root,
                "time");

        if (!cJSON_IsNumber(time))
        {
            ESP_LOGW(
                TAG,
                "Invalid server time");

            err = ESP_ERR_INVALID_ARG;
        }
        else
        {
            uint64_t server_time =
                (uint64_t)time->valuedouble;

            ESP_LOGD(
                TAG,
                "Server time: %" PRIu64,
                server_time);

            if (s_time_callback != NULL)
            {
                s_time_callback(server_time);
            }

            err = ESP_OK;
        }
    }

    else
    {
        ESP_LOGW(
            TAG,
            "Unknown protocol message: %s",
            type->valuestring);
    }

    cJSON_Delete(root);

    return err;
}

satellite_client_role_t satellite_client_protocol_get_role(void)
{
    return s_role;
}

bool satellite_client_protocol_has_role(void)
{
    return s_role != SATELLITE_CLIENT_ROLE_UNKNOWN;
}

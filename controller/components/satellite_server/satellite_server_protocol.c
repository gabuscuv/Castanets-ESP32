/// SHAMEFUL CODE

#include "satellite_server_protocol.h"

#include <stdbool.h>
#include <string.h>

#include "cJSON.h"
#include "esp_log.h"

#include "esp_mac.h"
#include "satellite_server_espnow_send.h"

static const char *TAG = "satellite_protocol";


typedef enum
{
    SATELLITE_MESSAGE_UNKNOWN = 0,

    SATELLITE_MESSAGE_REQUEST_CONTROLLER_ROLE,
    SATELLITE_MESSAGE_CLICK,
    SATELLITE_MESSAGE_IMU,

} satellite_message_type_t;


typedef enum
{
    SATELLITE_CONTROLLER_ROLE_UNKNOWN = 0,
    SATELLITE_CONTROLLER_ROLE_LEFT,
    SATELLITE_CONTROLLER_ROLE_RIGHT,

} satellite_controller_role_t;


typedef struct
{
    uint64_t time;
    satellite_controller_role_t role;
} satellite_click_message_t;


typedef struct
{
    uint64_t time;
    float x;
    float y;
    float z;
} satellite_imu_message_t;


/*
 * Keep protocol parsing separate from application state.
 *
 * This structure represents one parsed incoming message.
 * It is not the persistent controller state.
 */
typedef struct
{
    satellite_message_type_t type;

    union
    {
        satellite_click_message_t click;
        satellite_imu_message_t imu;
    };
} satellite_message_t;


/* --------------------------------------------------------------------------
 * Parsing helpers
 * -------------------------------------------------------------------------- */

static satellite_message_type_t
satellite_protocol_parse_type(const cJSON *type)
{
    if (!cJSON_IsString(type) || type->valuestring == NULL)
        return SATELLITE_MESSAGE_UNKNOWN;

    if (strcmp(type->valuestring, "request_controller_role") == 0)
        return SATELLITE_MESSAGE_REQUEST_CONTROLLER_ROLE;

    if (strcmp(type->valuestring, "click") == 0)
        return SATELLITE_MESSAGE_CLICK;

    if (strcmp(type->valuestring, "imu") == 0)
        return SATELLITE_MESSAGE_IMU;

    return SATELLITE_MESSAGE_UNKNOWN;
}


static satellite_controller_role_t
satellite_protocol_parse_role(const cJSON *role)
{
    if (!cJSON_IsString(role) || role->valuestring == NULL)
        return SATELLITE_CONTROLLER_ROLE_UNKNOWN;

    if (strcmp(role->valuestring, "Left") == 0)
        return SATELLITE_CONTROLLER_ROLE_LEFT;

    if (strcmp(role->valuestring, "Right") == 0)
        return SATELLITE_CONTROLLER_ROLE_RIGHT;

    return SATELLITE_CONTROLLER_ROLE_UNKNOWN;
}


static bool
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


static bool
satellite_protocol_parse_click(
    const cJSON *root,
    satellite_message_t *message)
{
    const cJSON *time = cJSON_GetObjectItemCaseSensitive(root, "time");
    const cJSON *role = cJSON_GetObjectItemCaseSensitive(root, "role");

    if (!satellite_protocol_parse_uint64(
            time,
            &message->click.time))
    {
        ESP_LOGW(TAG, "Invalid click time");
        return false;
    }

    message->click.role = satellite_protocol_parse_role(role);

    if (message->click.role == SATELLITE_CONTROLLER_ROLE_UNKNOWN)
    {
        ESP_LOGW(TAG, "Invalid click role");
        return false;
    }

    return true;
}


static bool
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


/*
 * Parse the JSON envelope first, then parse fields belonging to the
 * particular message type.
 */
static bool
satellite_protocol_parse(
    const uint8_t *data,
    uint16_t data_len,
    satellite_message_t *message,
    const cJSON **root_out)
{
    if (data == NULL || data_len == 0 || message == NULL)
        return false;

    /*
     * cJSON_ParseWithLengthOpts() allows the ESP-NOW payload to be
     * treated as a byte buffer without requiring the caller to append
     * a NUL terminator.
     */
    cJSON *root = cJSON_ParseWithLength(
        (const char *)data,
        data_len);

    if (root == NULL)
    {
        ESP_LOGW(TAG, "Invalid JSON packet");
        return false;
    }

    if (!cJSON_IsObject(root))
    {
        ESP_LOGW(TAG, "JSON packet is not an object");
        cJSON_Delete(root);
        return false;
    }

    const cJSON *type =
        cJSON_GetObjectItemCaseSensitive(root, "type");

    message->type = satellite_protocol_parse_type(type);

    bool valid = false;

    switch (message->type)
    {
        case SATELLITE_MESSAGE_REQUEST_CONTROLLER_ROLE:
            /*
             * No additional parsing yet.
             * The request currently only needs its presence/type.
             */
            valid = true;
            break;

        case SATELLITE_MESSAGE_CLICK:
            valid = satellite_protocol_parse_click(root, message);
            break;

        case SATELLITE_MESSAGE_IMU:
            valid = satellite_protocol_parse_imu(root, message);
            break;

        case SATELLITE_MESSAGE_UNKNOWN:
        default:
            ESP_LOGW(TAG, "Unknown message type");
            valid = false;
            break;
    }

    if (!valid)
    {
        cJSON_Delete(root);
        return false;
    }

    if (root_out != NULL)
        *root_out = root;
    else
        cJSON_Delete(root);

    return true;
}


/* --------------------------------------------------------------------------
 * Message handlers
 * -------------------------------------------------------------------------- */

static esp_err_t
satellite_protocol_handle_role_request(
    const uint8_t src_mac[ESP_NOW_ETH_ALEN],
    const cJSON *root)
{
    (void)root;

    ESP_LOGI(
        TAG,
        "Controller role requested by " MACSTR,
        MAC2STR(src_mac));

    /*
     * TODO:
     *
     * 1. Read/validate the requested "hwid".
     * 2. Ask satellite_server_state which role is available.
     * 3. Update the controller's assigned role.
     * 4. Build:
     *
     *    {
     *      "type": "reply_controller_role",
     *      "hwidclient": "...",
     *      "role": "Left"
     *    }
     *
     * 5. Send it with satellite_espnow_send().
     */

    return ESP_OK;
}


static esp_err_t
satellite_protocol_handle_click(
    const uint8_t src_mac[ESP_NOW_ETH_ALEN],
    const satellite_click_message_t *click)
{
    ESP_LOGD(
        TAG,
        "Click from " MACSTR ": time=%llu role=%d",
        MAC2STR(src_mac),
        (unsigned long long)click->time,
        (int)click->role);

    /*
     * TODO:
     * satellite_server_state_add_click(...);
     */

    return ESP_OK;
}


static esp_err_t
satellite_protocol_handle_imu(
    const uint8_t src_mac[ESP_NOW_ETH_ALEN],
    const satellite_imu_message_t *imu)
{
    ESP_LOGD(
        TAG,
        "IMU from : time=%llu x=%f y=%f z=%f",
        MAC2STR(src_mac),
        (unsigned long long)imu->time,
        imu->x,
        imu->y,
        imu->z);

    /*
     * TODO:
     * satellite_server_state_update_imu(...);
     */

    return ESP_OK;
}


/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

esp_err_t satellite_server_protocol_handle(
    const uint8_t src_mac[ESP_NOW_ETH_ALEN],
    const uint8_t *data,
    uint16_t data_len)
{
    if (src_mac == NULL ||
        data == NULL ||
        data_len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    satellite_message_t message;
    memset(&message, 0, sizeof(message));

    const cJSON *root = NULL;

    if (!satellite_protocol_parse(
            data,
            data_len,
            &message,
            &root))
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_OK;

    switch (message.type)
    {
        case SATELLITE_MESSAGE_REQUEST_CONTROLLER_ROLE:
            err = satellite_protocol_handle_role_request(
                src_mac,
                root);
            break;

        case SATELLITE_MESSAGE_CLICK:
            err = satellite_protocol_handle_click(
                src_mac,
                &message.click);
            break;

        case SATELLITE_MESSAGE_IMU:
            err = satellite_protocol_handle_imu(
                src_mac,
                &message.imu);
            break;

        case SATELLITE_MESSAGE_UNKNOWN:
        default:
            err = ESP_ERR_INVALID_ARG;
            break;
    }

    cJSON_Delete((cJSON *)root);

    return err;
}

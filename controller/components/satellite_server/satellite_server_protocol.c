#include "satellite_server_protocol.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "cJSON.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"

#include "satellite_espnow_protocol.h"
#include "satellite_server_espnow.h"
#include "satellite_server_espnow_send.h"
#include "satellite_server_clientmgnt.h"
#include "satellite_server_protocol_parsers.h"
#include "satellite_server_protocol_discovery.h"

static bool s_initialized= false;

static const char *TAG = "satellite_server_protocol";




/* -------------------------------------------------------------------------- */
/* JSON                                                                       */
/* -------------------------------------------------------------------------- */


static esp_err_t handle_json(
    const uint8_t client_mac[ESP_NOW_ETH_ALEN],
    const uint8_t *data,
    uint16_t data_len)
{
    if (client_mac == NULL ||
        data == NULL ||
        data_len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    satellite_role_t role;

    if (!satellite_server_protocol_get_role(client_mac, &role))
    {
        ESP_LOGW(
            TAG,
            "JSON packet from unknown client "
            MACSTR,
            MAC2STR(client_mac));

        return ESP_ERR_NOT_FOUND;
    }

    /*
        * IMPORTANT:
        *
        * data is not guaranteed to be NUL terminated.
        *
        * Therefore use ParseWithLength() and %.*s.
        */
    cJSON *json_root = cJSON_ParseWithLength(
    (const char *)data,
    data_len);

    ESP_LOGI(
        TAG,
        "MESSAGE from " MACSTR " role=%u: %.*s",
        MAC2STR(client_mac),
        role,
        (int)data_len,
        (const char *)data);

    if (json_root == NULL)
    {
        ESP_LOGW(
            TAG,
            "Invalid JSON packet from " MACSTR,
            MAC2STR(client_mac));

        return ESP_ERR_INVALID_ARG;
    }

    /* Allocate protocol message */
    satellite_message_t *msg_parsed = calloc(1, sizeof(*msg_parsed));

    if (msg_parsed == NULL)
    {
        cJSON_Delete(json_root);
        return ESP_ERR_NO_MEM;
    }

    /* Parse message type */
    const cJSON *type =
        cJSON_GetObjectItemCaseSensitive(json_root, "type");

    msg_parsed->type = satellite_protocol_parse_type(type);

    bool valid = false;

    switch (msg_parsed->type)
    {
        case SATELLITE_MSG_CLICK:
            valid = satellite_protocol_parse_click(json_root, msg_parsed);
            break;

        case SATELLITE_MSG_IMU:
            valid = satellite_protocol_parse_imu(json_root, msg_parsed);
            break;

        case SATELLITE_MSG_UNKNOWN:
        default:
            ESP_LOGW(TAG, "Unknown message type");
            valid = false;
            break;
    }

    /* cJSON is no longer needed */
    cJSON_Delete(json_root);

    if (!valid)
    {
        free(msg_parsed);
        return ESP_ERR_INVALID_ARG;
    }

    satellite_server_clientmgnt_get_client_role(client_mac,(satellite_role_t*)(msg_parsed->role));

    /* Transfer ownership to caller */
    /// CALLBACK
    
    free(msg_parsed);
    return ESP_OK;
}


/* -------------------------------------------------------------------------- */
/* RX entry point                                                             */
/* -------------------------------------------------------------------------- */

esp_err_t satellite_server_protocol_handle(
    const uint8_t client_mac[ESP_NOW_ETH_ALEN],
    const uint8_t *data,
    uint16_t data_len)
{
    if (client_mac == NULL ||
        data == NULL ||
        data_len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * Discovery is currently the only binary packet.
     *
     * sizeof(satellite_discover_packet_t) == 2:
     *
     *   [0] = SATELLITE_MSG_DISCOVER
     *   [1] = requested role
     */
    if (data_len == sizeof(satellite_discover_packet_t) &&
        data[0] == SATELLITE_MSG_DISCOVER)
    {
        const satellite_discover_packet_t *packet =
            (const satellite_discover_packet_t *)data;

        return satellite_protocol_handle_discovery(
            client_mac,
            packet);
    }

    /*
     * Everything that isn't discovery is currently considered
     * an application JSON packet.
     */
    return handle_json(
        client_mac,
        data,
        data_len);
}


/* -------------------------------------------------------------------------- */
/* Lifecycle                                                                  */
/* -------------------------------------------------------------------------- */

esp_err_t satellite_server_protocol_init(void)
{
    if (s_initialized){ return ESP_OK; }
    esp_err_t err;
    ESP_LOGI(TAG,"Initializing Satellite client management");

    err = satellite_server_clientmgnt_init();
    if (err != ESP_OK){return err;}
    ESP_LOGI(TAG, "Satellite protocol initialized");
    s_initialized = true;
    return ESP_OK;
}


esp_err_t satellite_server_protocol_deinit(void)
{
    if (s_initialized){ return ESP_OK; }
    esp_err_t err;

    err = satellite_server_clientmgnt_deinit(); 
    if (err != ESP_OK){return err;}

    return ESP_OK;
}


/* -------------------------------------------------------------------------- */
/* Public client lookup                                                       */
/* -------------------------------------------------------------------------- */

bool satellite_server_protocol_get_role(
    const uint8_t client_mac[ESP_NOW_ETH_ALEN],
    satellite_role_t *role)
{
    if (!s_initialized)
        return false;

    return satellite_server_clientmgnt_get_client_role(
        client_mac,
        role);
}
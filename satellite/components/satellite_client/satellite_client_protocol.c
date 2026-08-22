#include "satellite_client_protocol.h"

#include <string.h>
#include "cJSON.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "satellite_client_espnow.h"
#include "satellite_client_espnow_send.h"
#include "satellite_client_protocol_parser.h"
#include "satellite_client_protocol_types.h"
#include "satellite_espnow_protocol.h"

static const char *TAG = "satellite_client_protocol";
static satellite_client_role_t s_role = SATELLITE_CLIENT_ROLE_UNKNOWN;
static bool s_initialized;
satellite_client_protocol_callback_t s_time_callback = NULL;

static esp_err_t send_json(
    const uint8_t dest_mac[ESP_NOW_ETH_ALEN], cJSON *root)
{
    char *json = cJSON_PrintUnformatted(root);
    if (!json)
    {
        return ESP_ERR_NO_MEM;
    }

    size_t len = strlen(json);
    if (len > ESP_NOW_MAX_DATA_LEN)
    {
        cJSON_free(json);
        return ESP_ERR_INVALID_SIZE;
    }
    ESP_LOGI(TAG, "Sending JSON");
    esp_err_t err = satellite_espnow_send(
        dest_mac, (const uint8_t *)json, len);
    cJSON_free(json);
    return err;
}

esp_err_t satellite_client_protocol_init(satellite_client_protocol_callback_t time_callback)
{
    if (time_callback == NULL) {return ESP_ERR_INVALID_ARG;}
  
    s_time_callback = time_callback;
    s_role = SATELLITE_CLIENT_ROLE_UNKNOWN;
    s_initialized = true;

    return ESP_OK;
}

esp_err_t satellite_client_protocol_send_click(
    const uint8_t server_mac[ESP_NOW_ETH_ALEN],
    uint64_t time)
{
    if (!s_initialized){return ESP_ERR_INVALID_STATE;}
    if (!server_mac){return ESP_ERR_INVALID_ARG;}

    cJSON *root = cJSON_CreateObject();
    if (!root){return ESP_ERR_NO_MEM;}

    if (!cJSON_AddStringToObject(root, "type", "click") ||
    !cJSON_AddNumberToObject(root, "time", (double)time))
    {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = send_json(server_mac, root);
    cJSON_Delete(root);
    return err;
}

esp_err_t satellite_client_protocol_send_imu(
    const uint8_t server_mac[ESP_NOW_ETH_ALEN],
    uint64_t time,
    float x, float y, float z)
{
    if (!s_initialized){return ESP_ERR_INVALID_STATE;}
    if (!server_mac){return ESP_ERR_INVALID_ARG;}

    cJSON *root = cJSON_CreateObject();
    if (!root){return ESP_ERR_NO_MEM;}

    if (!cJSON_AddStringToObject(root, "type", "imu") ||
        !cJSON_AddNumberToObject(root, "time", (double)time) ||
        !cJSON_AddNumberToObject(root, "x", x) ||
        !cJSON_AddNumberToObject(root, "y", y) ||
        !cJSON_AddNumberToObject(root, "z", z))
    {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = send_json(server_mac, root);
    cJSON_Delete(root);
    return err;
}

esp_err_t satellite_client_protocol_json_handle(const uint8_t src_mac[ESP_NOW_ETH_ALEN], const uint8_t *data, uint16_t data_len)
{
    cJSON *root = cJSON_ParseWithLength((const char *)data, data_len);

    if (root == NULL || !cJSON_IsObject(root))
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err;
    /*
     * Controller role assignment
     */
    satellite_message_tt satellite_msg;

    err = satellite_client_protocol_parse(root, &satellite_msg);

    cJSON_Delete(root);
    
    if(err != ESP_OK)
    {
        return ESP_ERR_INVALID_ARG;
    }

    s_time_callback(satellite_msg);

    return err;
}

esp_err_t
satellite_client_protocol_handle(const uint8_t src_mac[ESP_NOW_ETH_ALEN],
                                 const uint8_t *data, uint16_t data_len)
{

    if (!s_initialized){return ESP_ERR_INVALID_STATE;}
    if (src_mac == NULL || data == NULL || data_len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const uint8_t type = data[0];

    if (type == SATELLITE_MSG_ASSIGN)
    {
        if (data_len < sizeof(satellite_assign_packet_t))
            return ESP_ERR_INVALID_SIZE;

        const satellite_assign_packet_t *assignment =
            (const satellite_assign_packet_t *)data;

        esp_err_t err =
            satellite_espnow_add_peer(src_mac);

        if (err != ESP_OK)
        {
            ESP_LOGE(
                TAG,
                "Failed to add server peer: %s",
                esp_err_to_name(err));

            return err;
        }

        satellite_message_tt satellite_msg;
        satellite_msg.type = CONTROLLER_ACK_ROLE;
        satellite_msg.ack_controller.role = assignment->role;
        s_time_callback(satellite_msg);

        s_role = assignment->role;

        ESP_LOGI(
            TAG,
            "Connected to server " MACSTR " with role %u",
            MAC2STR(src_mac),
            s_role);

        return ESP_OK;
    }


    return satellite_client_protocol_json_handle(src_mac,data,data_len);
}


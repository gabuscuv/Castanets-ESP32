#include "satellite_server_protocol_discovery.h"

#include "esp_log.h"

#include "satellite_server_clientmgnt.h"
#include "satellite_server_espnow_send.h"

static const char *TAG = "satellite_server_protocol_discovery";

esp_err_t satellite_protocol_handle_discovery(
    const uint8_t client_mac[ESP_NOW_ETH_ALEN],
    const satellite_discover_packet_t *packet)
{
    if (client_mac == NULL || packet == NULL)
        return ESP_ERR_INVALID_ARG;

    ESP_LOGI(
        TAG,
        "DISCOVER from " MACSTR " requested_role=%u",
        MAC2STR(client_mac),
        packet->requested_role);


    satellite_role_t assigned_role = packet->requested_role;

    if (packet->requested_role == SATELLITE_CONTROLLER_ROLE_UNKNOWN)
    {
        assigned_role = satellite_server_clientmgnt_get_role_available();
    }
    
    esp_err_t err = satellite_server_clientmgnt_register_client(
        client_mac,
        assigned_role);

    if (err != ESP_OK)
        return err;

    /*
     * Send the assignment back to the client.
     *
     * satellite_server_espnow_send_assignment() is responsible for
     * adding the client as a peer and sending the packet.
     */
    err = satellite_server_espnow_send_assignment(
        client_mac,
        assigned_role);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to send assignment to "
            MACSTR ": %s",
            MAC2STR(client_mac),
            esp_err_to_name(err));

        return err;
    }

    ESP_LOGI(
        TAG,
        "Assigned " MACSTR " role=%u",
        MAC2STR(client_mac),
        assigned_role);

    return ESP_OK;
}

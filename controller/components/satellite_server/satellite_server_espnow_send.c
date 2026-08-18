#include "satellite_server_espnow_send.h"

#include <stdbool.h>
#include <string.h>

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_wifi.h"

#include "ESPNOW_CONFIG.h"


static const char *TAG = "satellite_server_send";


/* -------------------------------------------------------------------------- */
/* Peer management                                                            */
/* -------------------------------------------------------------------------- */

static esp_err_t satellite_server_add_peer(
    const uint8_t mac[ESP_NOW_ETH_ALEN])
{
    if (mac == NULL)
        return ESP_ERR_INVALID_ARG;

    if (esp_now_is_peer_exist(mac))
        return ESP_OK;

    esp_now_peer_info_t peer = {0};

    memcpy(
        peer.peer_addr,
        mac,
        ESP_NOW_ETH_ALEN);

    peer.channel = CONFIG_ESPNOW_CHANNEL;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;

    esp_err_t err = esp_now_add_peer(&peer);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to add peer " MACSTR ": %s",
            MAC2STR(mac),
            esp_err_to_name(err));

        return err;
    }

    ESP_LOGI(
        TAG,
        "Added ESP-NOW peer " MACSTR,
        MAC2STR(mac));

    return ESP_OK;
}


/* -------------------------------------------------------------------------- */
/* Assignment                                                                 */
/* -------------------------------------------------------------------------- */

esp_err_t satellite_server_espnow_send_assignment(
    const uint8_t client_mac[ESP_NOW_ETH_ALEN],
    satellite_role_t role)
{
    if (client_mac == NULL)
        return ESP_ERR_INVALID_ARG;

    /*
     * The client MAC came from the ESP-NOW discovery packet, so it
     * is the MAC address we need to add as a unicast peer.
     */
    esp_err_t err = satellite_server_add_peer(client_mac);

    if (err != ESP_OK)
        return err;


    /*
     * Get our own STA MAC.
     *
     * The client needs this address so that subsequent packets can
     * be sent directly to the server instead of being broadcast.
     */
    satellite_assign_packet_t packet = {
        .type = SATELLITE_MSG_ASSIGN,
        .role = role,
    };

    err = esp_wifi_get_mac(
        WIFI_IF_STA,
        packet.server_mac);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to get server STA MAC: %s",
            esp_err_to_name(err));

        return err;
    }


    ESP_LOGI(
        TAG,
        "Sending assignment to " MACSTR
        ": role=%u server=" MACSTR,
        MAC2STR(client_mac),
        role,
        MAC2STR(packet.server_mac));


    /*
     * Send the small binary assignment packet.
     */
    err = esp_now_send(
        client_mac,
        (const uint8_t *)&packet,
        sizeof(packet));

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to send assignment to " MACSTR ": %s",
            MAC2STR(client_mac),
            esp_err_to_name(err));

        return err;
    }

    return ESP_OK;
}
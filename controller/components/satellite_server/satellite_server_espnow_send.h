#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "esp_now.h"

#include "satellite_espnow_protocol.h"


/**
 * Send a role/server assignment to a satellite client.
 *
 * The client MAC is learned dynamically during discovery.
 *
 * The packet contains:
 *   - assigned role
 *   - server STA MAC address
 *
 * The client is automatically added as an unencrypted ESP-NOW peer
 * if it is not already registered.
 */
esp_err_t satellite_server_espnow_send_assignment(
    const uint8_t client_mac[ESP_NOW_ETH_ALEN], satellite_role_t role);

esp_err_t satellite_server_espnow_send_json(
    const uint8_t client_mac[ESP_NOW_ETH_ALEN],
    const char *json,
    size_t json_len);

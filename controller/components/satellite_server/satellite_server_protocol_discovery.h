#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "esp_mac.h"

#include "satellite_espnow_protocol.h"

/**
 * Handle satellite discovery packet from client
 */
esp_err_t satellite_protocol_handle_discovery(
    const uint8_t client_mac[ESP_NOW_ETH_ALEN],
    const satellite_discover_packet_t *packet);


#pragma once

#include <stdint.h>

#include "esp_now.h"
#include "esp_err.h"

/*
 * Entry point for messages received from a satellite/controller.
 *
 * This function is called by the ESP-NOW RX worker task, not directly
 * from the ESP-NOW Wi-Fi callback.
 */
esp_err_t satellite_server_protocol_handle(
    const uint8_t src_mac[ESP_NOW_ETH_ALEN],
    const uint8_t *data,
    uint16_t data_len);

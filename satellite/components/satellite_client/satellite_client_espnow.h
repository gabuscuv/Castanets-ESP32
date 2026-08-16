#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_now.h"

typedef esp_err_t (*satellite_client_espnow_packet_callback_t)(
    const uint8_t src_mac[ESP_NOW_ETH_ALEN],
    const uint8_t *data,
    uint16_t data_len);

esp_err_t satellite_client_espnow_init(
    satellite_client_espnow_packet_callback_t recv_cb);

esp_err_t satellite_client_espnow_send(
    const uint8_t dest_mac[ESP_NOW_ETH_ALEN],
    const uint8_t *data,
    size_t data_len);

esp_err_t satellite_client_espnow_send_broadcast(
    const uint8_t *data,
    size_t data_len);

esp_err_t satellite_client_espnow_deinit(void);

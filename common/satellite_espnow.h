#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_now.h"


typedef esp_err_t (*satellite_espnow_packet_callback_t)(
    const uint8_t src_mac[ESP_NOW_ETH_ALEN],
    const uint8_t *data,
    uint16_t data_len);


esp_err_t satellite_espnow_init(
    satellite_espnow_packet_callback_t recv_cb);


esp_err_t satellite_espnow_deinit(void);


esp_err_t satellite_espnow_send(
    const uint8_t dest_mac[ESP_NOW_ETH_ALEN],
    const uint8_t *data,
    size_t data_len);


esp_err_t satellite_espnow_send_broadcast(
    const uint8_t *data,
    size_t data_len);


esp_err_t satellite_espnow_add_peer(
    const uint8_t mac[ESP_NOW_ETH_ALEN]);


bool satellite_espnow_peer_exists(
    const uint8_t mac[ESP_NOW_ETH_ALEN]);
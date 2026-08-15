#pragma once

#include "esp_now.h"
typedef esp_err_t (*satellite_espnow_packet_callback_t)(
    const uint8_t src_mac[ESP_NOW_ETH_ALEN],
    const uint8_t *data,
    uint16_t data_len);

esp_err_t satellite_espnow_init(satellite_espnow_packet_callback_t recv_cb);
esp_err_t satellite_espnow_deinit(void);
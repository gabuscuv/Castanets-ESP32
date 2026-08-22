#pragma once

#include "esp_now.h"

esp_err_t satellite_espnow_send(const uint8_t dest_mac[ESP_NOW_ETH_ALEN], const uint8_t *data, size_t data_len);

esp_err_t satellite_espnow_send_broadcast(const uint8_t *data, size_t data_len);
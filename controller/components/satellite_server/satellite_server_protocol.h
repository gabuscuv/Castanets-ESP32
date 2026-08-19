#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_now.h"

#include "satellite_espnow_protocol.h"

typedef esp_err_t (*satellite_protocol_callback_t)(satellite_message_t);

esp_err_t satellite_server_protocol_init(satellite_protocol_callback_t satellite_cb);

esp_err_t satellite_server_protocol_deinit(void);

esp_err_t satellite_server_protocol_handle(
    const uint8_t client_mac[ESP_NOW_ETH_ALEN],
    const uint8_t *data,
    uint16_t data_len);

bool satellite_server_protocol_get_role(
    const uint8_t client_mac[ESP_NOW_ETH_ALEN], satellite_role_t *role);

esp_err_t satellite_server_protocol_reset_satellites_time();

esp_err_t satellite_server_protocol_push_time(uint32_t time);

esp_err_t satellite_server_protocol_request_status();
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_now.h"

#include "satellite_espnow.h"
#include "satellite_espnow_protocol.h"


esp_err_t satellite_client_espnow_init(
    satellite_espnow_packet_callback_t recv_cb,
    satellite_role_t requested_role);


esp_err_t satellite_client_espnow_deinit(void);


esp_err_t satellite_client_espnow_send(
    const uint8_t *data,
    size_t data_len);


bool satellite_client_espnow_is_connected(void);


satellite_role_t satellite_client_espnow_get_role(void);


bool satellite_client_espnow_get_server_mac(
    uint8_t mac[ESP_NOW_ETH_ALEN]);
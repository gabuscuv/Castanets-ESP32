#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_now.h"

#include "satellite_espnow.h"


typedef uint8_t satellite_role_t;

#define SATELLITE_ROLE_NONE 0


/*
 * Called when a previously unknown client requests registration.
 *
 * requested_role:
 *     0 = client has no preference.
 *
 * Return:
 *     assigned role.
 *
 * Return SATELLITE_ROLE_NONE to reject the client.
 */
typedef satellite_role_t (*satellite_server_role_assign_callback_t)(
    const uint8_t client_mac[ESP_NOW_ETH_ALEN],
    satellite_role_t requested_role);


esp_err_t satellite_server_espnow_init(
    satellite_espnow_packet_callback_t recv_cb,
    satellite_server_role_assign_callback_t role_assign_cb);


esp_err_t satellite_server_espnow_deinit(void);


bool satellite_server_espnow_get_role(
    const uint8_t client_mac[ESP_NOW_ETH_ALEN],
    satellite_role_t *role);
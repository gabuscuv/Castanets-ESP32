#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_now.h"

typedef enum {
    SATELLITE_CLIENT_ROLE_UNKNOWN = 0,
    SATELLITE_CLIENT_ROLE_LEFT,
    SATELLITE_CLIENT_ROLE_RIGHT,
} satellite_client_role_t;

typedef void (*satellite_client_protocol_time_callback_t)(uint64_t time);
/*
 * Initialize protocol state and register the callback that is notified
 * when a server-time message is received.
 */
esp_err_t satellite_client_protocol_init(satellite_client_protocol_time_callback_t time_callback);


/*
 * Send a request asking the server to assign a controller role.
 */
esp_err_t satellite_client_protocol_request_role(const uint8_t server_mac[ESP_NOW_ETH_ALEN]);


/*
 * Process a received ESP-NOW packet.
 */
esp_err_t satellite_client_protocol_handle(const uint8_t src_mac[ESP_NOW_ETH_ALEN], const uint8_t *data, uint16_t data_len);


/*
 * Send controller input.
 */
esp_err_t satellite_client_protocol_send_click(const uint8_t server_mac[ESP_NOW_ETH_ALEN], uint64_t time);


esp_err_t satellite_client_protocol_send_imu(
    const uint8_t server_mac[ESP_NOW_ETH_ALEN],
    uint64_t time,
    float x,
    float y,
    float z);


/*
 * Controller role assigned by the server.
 */
satellite_client_role_t satellite_client_protocol_get_role(void);

bool satellite_client_protocol_has_role(void);
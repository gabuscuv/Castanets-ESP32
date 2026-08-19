#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "satellite_espnow_protocol.h"


typedef struct
{
    uint8_t mac[ESP_NOW_ETH_ALEN];
    satellite_role_t role;
} satellite_client_info_t;


#define SATELLITE_SERVER_MAX_CLIENTS 3

esp_err_t satellite_server_clientmgnt_init();
esp_err_t satellite_server_clientmgnt_deinit();
esp_err_t
satellite_server_clientmgnt_register_client(const uint8_t mac[ESP_NOW_ETH_ALEN],
                                            satellite_role_t role);
satellite_role_t satellite_server_clientmgnt_get_role_available();

bool satellite_server_clientmgnt_get_client_role(
    const uint8_t mac[ESP_NOW_ETH_ALEN], satellite_role_t *role);
size_t satellite_server_clientmgnt_get_clients(satellite_client_info_t *clients, size_t max_clients);
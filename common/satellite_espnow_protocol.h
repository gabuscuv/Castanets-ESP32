#pragma once

#include <stdint.h>

#include "esp_now.h"


typedef uint8_t satellite_role_t;

#define SATELLITE_ROLE_NONE 0


typedef enum
{
    SATELLITE_MSG_DISCOVER = 1,
    SATELLITE_MSG_ASSIGN   = 2,
    SATELLITE_MSG_DATA     = 3,
} satellite_message_type_t;


/*
 * Client -> Server
 */
typedef struct __attribute__((packed))
{
    uint8_t type;
    satellite_role_t requested_role;
} satellite_discover_packet_t;


/*
 * Server -> Client
 */
typedef struct __attribute__((packed))
{
    uint8_t type;
    satellite_role_t role;

    uint8_t server_mac[ESP_NOW_ETH_ALEN];

} satellite_assign_packet_t;
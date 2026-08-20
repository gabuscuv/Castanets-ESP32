#pragma once
#include "controller_msg.h"
#include "esp_now.h"

typedef enum
{
    SATELLITE_CLIENT_ROLE_UNKNOWN = 0,
    SATELLITE_CLIENT_ROLE_LEFT,
    SATELLITE_CLIENT_ROLE_RIGHT,
} satellite_client_role_t;

typedef struct
{
    uint8_t hw_server[ESP_NOW_ETH_ALEN];
    satellite_client_role_t role;
} controller_serverack_message_type_t;

typedef struct
{
    controller_message_type_t type;

    union
    {
        controller_serverack_message_type_t ack_controller;
        int64_t time;
    } ;
} satellite_message_tt;
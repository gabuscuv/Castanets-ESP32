#pragma once

#include <stdint.h>

#include "esp_now.h"


typedef uint8_t satellite_role_t;

#define SATELLITE_ROLE_NONE 0


typedef enum {
    SATELLITE_MSG_UNKNOWN = -1,
    SATELLITE_MSG_DISCOVER = 1,
    SATELLITE_MSG_ASSIGN = 2,
    SATELLITE_MSG_CLICK,
    SATELLITE_MSG_IMU,
} satellite_message_type_t;

typedef enum
{
    SATELLITE_CONTROLLER_ROLE_UNKNOWN = 0,
    SATELLITE_CONTROLLER_ROLE_LEFT,
    SATELLITE_CONTROLLER_ROLE_RIGHT,

} satellite_controller_role_t;


typedef struct
{
    uint64_t time;
} satellite_click_message_t;


typedef struct
{
    uint64_t time;
    float x;
    float y;
    float z;
} satellite_imu_message_t;


typedef struct
{
    satellite_controller_role_t role;
    satellite_message_type_t type;

    union
    {
        satellite_click_message_t click;
        satellite_imu_message_t imu;
    };
} satellite_message_t;

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

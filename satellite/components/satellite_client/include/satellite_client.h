#pragma once

#include "esp_err.h"
#include "controller_msg.h"

typedef struct
{
    controller_message_type_t type;

    union
    {
        int64_t time;
    } ;
} satellite_message_runtime_t;

typedef esp_err_t (*satellite_client_time_server_callback_t)(satellite_message_runtime_t);

esp_err_t satellite_client_init(satellite_client_time_server_callback_t time_callback);
esp_err_t satellite_client_push_click(uint64_t time);
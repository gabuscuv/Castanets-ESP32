#pragma once

#include "esp_err.h"
#include "controller_msg.h"

typedef void (*satellite_client_time_server_callback_t)(controller_message_type_t, uint32_t time);

esp_err_t satellite_client_init(satellite_client_time_server_callback_t time_callback);
esp_err_t satellite_client_push_click(uint64_t time);
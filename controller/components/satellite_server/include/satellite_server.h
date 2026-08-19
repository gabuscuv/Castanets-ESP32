#pragma once
#include "esp_err.h"
#include "controller_role.h"
#include <stdbool.h>
#include <stdint.h>


typedef esp_err_t (*satellite_push_callback_t)(controller_role_t, uint32_t);

esp_err_t satellite_server_init(satellite_push_callback_t st_push_cb);
esp_err_t satellite_server_deinit();

bool satellite_server_is_initialized();

esp_err_t satellite_server_reset_satellites_time();
esp_err_t satellite_server_push_time(uint32_t time);
esp_err_t satellite_server_request_status();
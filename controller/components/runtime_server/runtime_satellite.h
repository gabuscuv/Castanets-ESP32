
#pragma once
#include "esp_err.h"
#include "controller_role.h"
esp_err_t runtime_satellite_push_callback(controller_role_t controller, uint32_t time);
esp_err_t runtime_satellite_init();
esp_err_t runtime_satellite_deinit();
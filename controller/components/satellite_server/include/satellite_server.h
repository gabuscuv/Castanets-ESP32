#pragma once
#include "esp_err.h"

typedef enum {
  CONTROLLER_UNKNOWN = -1,
  CONTROLLER_LEFT,
  CONTROLLER_RIGHT,
} controller_role_t;

typedef esp_err_t (*satellite_push_callback_t)(controller_role_t, uint32_t);

esp_err_t satellite_server_init(satellite_push_callback_t st_push_cb);
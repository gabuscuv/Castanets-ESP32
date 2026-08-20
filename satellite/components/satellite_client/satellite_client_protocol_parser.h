#pragma once
#include "cJSON.h"
#include "esp_err.h"
#include "satellite_client_protocol_types.h"
esp_err_t satellite_client_protocol_parse(const cJSON *message, satellite_message_tt *out);
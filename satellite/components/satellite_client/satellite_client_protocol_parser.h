#pragma once
#include "cJSON.h"
#include "esp_err.h"
#include "satellite_espnow_protocol.h"
esp_err_t satellite_client_protocol_parse(const cJSON *message, satellite_message_t *out);
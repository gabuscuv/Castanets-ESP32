#pragma once
#include "esp_err.h"
esp_err_t satellite_client_protocol_parse(const cJSON *messag, satellite_message_t *out)
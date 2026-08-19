#pragma once

#include "cJSON.h"
#include "esp_err.h"
#include "inputframe.h"
#include "protocol_message.h"

cJSON *input_frame_to_json(const InputFrame *frame);
esp_err_t pccomm_cmd_from_json(const cJSON *json, pc_message_t *cmd);
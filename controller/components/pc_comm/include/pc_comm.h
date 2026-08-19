#pragma once
#include "esp_err.h"
#include "inputframe.h"
#include "protocol_message.h"

typedef esp_err_t (*pccomm_push_callback_t)(pc_message_t);

esp_err_t pccomm_init(pccomm_push_callback_t callback);
esp_err_t pccomm_deinit();
esp_err_t pccomm_sendInputFrame(InputFrame* inputFrame);
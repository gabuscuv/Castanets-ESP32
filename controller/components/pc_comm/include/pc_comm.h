#pragma once
#include "esp_err.h"
#include "inputframe.h"
esp_err_t pc_comm_init();
esp_err_t pc_comm_deinit();
esp_err_t pc_comm_send(InputFrame inputFrame);
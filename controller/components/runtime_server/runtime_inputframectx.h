#pragma once
#include "esp_err.h"
#include "inputframe.h"

esp_err_t runtime_inputframectx_init();
esp_err_t runtime_inputframectx_deinit();
InputFrame* runtime_inputframectx_get();
ControllerState* runtime_inputframectx_get_controller();
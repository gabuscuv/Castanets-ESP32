#pragma once

#include "esp_err.h"
typedef void (*piezocontroller_click_callback_t)();

esp_err_t piezocontroller_init(piezocontroller_click_callback_t click_callback);
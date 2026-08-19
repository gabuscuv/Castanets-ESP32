#pragma once
#include "esp_err.h"
#include "tinyusb_cdc_acm.h"
esp_err_t usbdevice_init(tusb_cdcacm_callback_t callback_rx);
esp_err_t usbdevice_deinit();


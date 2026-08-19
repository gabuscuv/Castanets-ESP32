#pragma once
#include "tinyusb_cdc_acm.h"

void usbdevice_serial_rx_callback(int itf, cdcacm_event_t *event);
esp_err_t usbdevice_serial_rx_init(void);
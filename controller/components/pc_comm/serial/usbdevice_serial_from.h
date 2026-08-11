#pragma once
#include "tinyusb_cdc_acm.h"

void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event);

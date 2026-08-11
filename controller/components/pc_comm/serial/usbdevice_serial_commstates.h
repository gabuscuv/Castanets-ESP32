#pragma once
#include "tinyusb_cdc_acm.h"


void tinyusb_cdc_line_state_changed_callback(int itf, cdcacm_event_t *event);

#pragma once
#include "tinyusb_cdc_acm.h"


void usbdevice_serial_commstates_changed(int itf, cdcacm_event_t *event);

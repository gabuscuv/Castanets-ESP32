#include "serial/serial_init.h"
int pc_comm_init() {
  // For now, Just Serial;
  return usbdevice_init();
}
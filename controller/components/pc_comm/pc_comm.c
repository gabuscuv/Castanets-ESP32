#include "pc_comm.h"
#include "serial/serial_init.h"
esp_err_t pc_comm_init() {
  // For now, Just Serial;
  return usbdevice_init();
}
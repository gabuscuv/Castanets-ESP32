#include "pc_comm.h"
#include "serial/serial_init.h"
#include "serial/usbdevice_serial_to.h"
esp_err_t pc_comm_init() {
  // For now, Just Serial;
  return usbdevice_init();
}

esp_err_t pc_comm_send(InputFrame inputFrame) {
  ;
  return serial_send(inputFrame);
}
#include "pc_comm.h"
#include "esp_err.h"
#include "serial/serial_init.h"
#include "pc_comm_protocol.h"
#include "serial/usbdevice_serial_rx.h"
#include "protocol_message.h"

pccomm_push_callback_t function_callback;

esp_err_t pccom_protocol_callback(pc_message_t callback)
{
  return function_callback(callback);
}

esp_err_t pccomm_init(pccomm_push_callback_t callback) {
  if (callback == NULL) { return ESP_ERR_INVALID_ARG;}
  
  pccomm_protocol_init(pccom_protocol_callback);
  usbdevice_serial_rx_init();
  
  return usbdevice_init(usbdevice_serial_rx_callback);
}

esp_err_t pccomm_deinit()
{
  return usbdevice_deinit();
}

esp_err_t pccomm_sendInputFrame(InputFrame* inputFrame) {
  return pccomm_protocol_sendFrame(inputFrame);
    
}
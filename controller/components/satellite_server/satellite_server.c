#include "satellite_server.h"

#include "esp_err.h"
#include "satellite_espnow_protocol.h"
#include "satellite_server_wifi.h"
#include "satellite_server_espnow.h"
// #include "satellite_server_espnow_send.h"
#include "satellite_server_protocol.h"
#include <stdint.h>

static bool s_initialized = false;
satellite_push_callback_t controller_callback;

esp_err_t satellite_protocol_callback(satellite_message_t message) {
  if (message.role == SATELLITE_CONTROLLER_ROLE_UNKNOWN) {
    return ESP_OK;
    }
    switch (message.type)
    {


    case SATELLITE_MSG_CLICK:
        controller_callback(
          message.role == SATELLITE_CONTROLLER_ROLE_LEFT
            ? CONTROLLER_LEFT
            : CONTROLLER_RIGHT,
            message.click.time
        );
      break;
    case SATELLITE_MSG_IMU:
      break;
      
    case SATELLITE_MSG_UNKNOWN:
    case SATELLITE_MSG_DISCOVER:
    case SATELLITE_MSG_ASSIGN:
        break;
    }
    return ESP_OK;
}

esp_err_t satellite_server_init(satellite_push_callback_t st_push_cb)
{
    if (st_push_cb == NULL){ return ESP_ERR_INVALID_ARG; }
    controller_callback = st_push_cb;

    esp_err_t err;
    
    err = satellite_server_protocol_init(satellite_protocol_callback);
    if (err != ESP_OK) { return err; }

    err = satellite_server_wifi_init();
    if (err != ESP_OK) { return err; }

    err = satellite_espnow_init(satellite_server_protocol_handle);
    if (err != ESP_OK) { return err; }

    s_initialized = true;
    return ESP_OK;
}

esp_err_t satellite_server_deinit(void)
{
    esp_err_t err;

    err = satellite_espnow_deinit();
    if (err != ESP_OK) {return err;}

    err = satellite_server_wifi_deinit();
    if (err != ESP_OK) {return err;}

    err = satellite_server_protocol_deinit();

    if (err != ESP_OK){return err;}

    controller_callback = NULL;
    
    s_initialized = false;

    return ESP_OK;
}

bool satellite_server_is_initialized() { return s_initialized; }

esp_err_t satellite_server_reset_satellites_time()
{
    if(!s_initialized){return ESP_ERR_INVALID_STATE;}
    return satellite_server_protocol_reset_satellites_time();
}

esp_err_t satellite_server_push_time(uint32_t time)
{
  if(!s_initialized){return ESP_ERR_INVALID_STATE;}
  return satellite_server_protocol_push_time(time);
}

esp_err_t satellite_server_request_status()
{
    if(!s_initialized){return ESP_ERR_INVALID_STATE;}
    return satellite_server_protocol_request_status();
}
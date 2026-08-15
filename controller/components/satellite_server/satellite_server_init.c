#include "satellite_server_init.h"

#include "satellite_server_wifi_init.h"
#include "satellite_server_espnow.h"
#include "satellite_server_espnow_send.h"
#include "satellite_server_protocol.h"

esp_err_t satellite_server_init(void)
{
    esp_err_t err;

    err = satellite_wifi_init();
    if (err != ESP_OK)
        return err;

    err = satellite_espnow_init(satellite_server_protocol_handle);
    if (err != ESP_OK)
      return err;

    err = satellite_espnow_send_init();
    if (err != ESP_OK)
        return err;

    return ESP_OK;
}
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_mac.h"

#include "esp_log.h"
#include "satellite_client_wifi.h"
#include "ESPNOW_CONFIG.h"

/* WiFi should start before using ESPNOW */
esp_err_t satellite_client_wifi_init(void)
{
    esp_err_t err;

    err = esp_netif_init();
    if (err != ESP_OK){ return err; }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE){ return err; }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    err = esp_wifi_init(&cfg);
    if (err != ESP_OK){ return err; }

    err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (err != ESP_OK){ return err; }

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK){ return err; }

    err = esp_wifi_start();
    if (err != ESP_OK){ return err; }

    err = esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
    if (err != ESP_OK){ return err; }


#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    err = esp_wifi_set_protocol(
        WIFI_IF_STA,
        WIFI_PROTOCOL_11B |
        WIFI_PROTOCOL_11G |
        WIFI_PROTOCOL_11N |
        WIFI_PROTOCOL_LR);

    if (err != ESP_OK){ return err; }
#endif

    return ESP_OK;
}
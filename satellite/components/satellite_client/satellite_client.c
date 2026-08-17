#include "satellite_client.h"

#include <string.h>

#include "esp_log.h"
#include "esp_now.h"
#include "esp_mac.h"

#include "satellite_client_wifi.h"
#include "satellite_client_espnow.h"
#include "satellite_client_protocol.h"

static const char *TAG = "satellite_client";

static uint8_t s_server_mac[ESP_NOW_ETH_ALEN];
static bool s_server_mac_valid = false;

static satellite_client_time_server_callback_t s_time_callback = NULL;

static void satellite_client_protocol_time_callback(uint64_t time)
{
    if (s_time_callback != NULL)
    {
        s_time_callback(time);
    }
}

esp_err_t satellite_client_init(
    satellite_client_time_server_callback_t time_callback)
{
    esp_err_t err;

    s_time_callback = time_callback;

    err = satellite_client_protocol_init(satellite_client_protocol_time_callback);

    if (err != ESP_OK)
    {
        s_time_callback = NULL;
        return err;
    }

    err = satellite_client_wifi_init();
    if (err != ESP_OK)
    {
        s_time_callback = NULL;
        return err;
    }

err = satellite_client_espnow_init(
satellite_client_protocol_handle,
    SATELLITE_ROLE_NONE);
    if (err != ESP_OK)
    {
        s_time_callback = NULL;
        return err;
    }

    ESP_LOGI(TAG, "Satellite client initialized");

    return ESP_OK;
}

esp_err_t satellite_client_set_server(
    const uint8_t server_mac[ESP_NOW_ETH_ALEN])
{
    if (server_mac == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(
        s_server_mac,
        server_mac,
        ESP_NOW_ETH_ALEN);

    s_server_mac_valid = true;

    ESP_LOGI(
        TAG,
        "Server set to " MACSTR,
        MAC2STR(s_server_mac));

    return ESP_OK;
}


esp_err_t satellite_client_push_click(uint64_t time)
{
    if (!s_server_mac_valid)
    {
        ESP_LOGW(
            TAG,
            "Cannot send click: server not configured");

        return ESP_ERR_INVALID_STATE;
    }

    return satellite_client_protocol_send_click(
        s_server_mac,
        time);
}
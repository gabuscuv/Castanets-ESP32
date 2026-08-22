#include "satellite_client_espnow.h"

#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_mac.h"

#include "ESPNOW_CONFIG.h"
#include "satellite_client_protocol_discovery.h"
#include "satellite_espnow_protocol.h"

static const char *TAG = "satellite_client";

static TaskHandle_t s_discovery_task = NULL;

static satellite_espnow_packet_callback_t s_recv_callback = NULL;

static uint8_t s_server_mac[ESP_NOW_ETH_ALEN];

static satellite_role_t s_requested_role = SATELLITE_ROLE_NONE;
static satellite_role_t s_role = SATELLITE_ROLE_NONE;

static volatile bool s_connected = false;
static volatile TickType_t s_last_server_packet = 0;

/* -------------------------------------------------------------------------- */
/* RX                                                                         */
/* -------------------------------------------------------------------------- */

static esp_err_t satellite_client_rx(
    const uint8_t src_mac[ESP_NOW_ETH_ALEN],
    const uint8_t *data,
    uint16_t data_len)
{
    if (src_mac == NULL || data == NULL || data_len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_connected)
    {
        s_last_server_packet = xTaskGetTickCount();
    }
    
    if (s_recv_callback != NULL)
    {
        return s_recv_callback(
            src_mac,
            data,
            data_len);
    
    }

    return ESP_OK;
}

void satellite_client_espnow_timeout()
{
    s_connected = 0;
    s_last_server_packet = 0;
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

esp_err_t satellite_client_espnow_init(
    satellite_espnow_packet_callback_t recv_cb,
    satellite_role_t requested_role)
{
  if (recv_cb == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    
    s_recv_callback = recv_cb;

    s_requested_role = requested_role;

    s_role = SATELLITE_ROLE_NONE;
    s_connected = false;

    memset(
        s_server_mac,
        0,
        sizeof(s_server_mac));

    esp_err_t err = satellite_espnow_init(satellite_client_rx);

    if (err != ESP_OK)
    {
        s_recv_callback = NULL;
        return err;
    }

    discovery_args args = {.connected_ptr = &s_connected,
                           .last_server_packet_ptr = &s_last_server_packet,
                           .timeout_function_callback =
                               satellite_client_espnow_timeout};

    err = satellite_client_discovery_init(args);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to initialize discovery: %s",
            esp_err_to_name(err));

        satellite_espnow_deinit();
        s_recv_callback = NULL;

        return err;
    }

    if (
        xTaskCreate(
            satellite_client_discovery_task,
            "sat_discovery",
            3072,
            NULL,
            4,
            &s_discovery_task) != pdPASS)
    {
        ESP_LOGE(
            TAG,
            "Failed to create discovery task");

        satellite_espnow_deinit();

        s_recv_callback = NULL;

        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(
        TAG,
        "Client ESP-NOW initialized");

    return ESP_OK;
}


esp_err_t satellite_client_espnow_set_connected(bool connected)
{
    s_connected = connected;
    s_last_server_packet = connected ? xTaskGetTickCount() : 0;
    return ESP_OK;
}

esp_err_t satellite_client_espnow_deinit(void)
{
    if (s_discovery_task != NULL)
    {
        vTaskDelete(s_discovery_task);
        s_discovery_task = NULL;
    }

    s_connected = false;
    s_role = SATELLITE_ROLE_NONE;

    memset(
        s_server_mac,
        0,
        sizeof(s_server_mac));

    s_recv_callback = NULL;

    return satellite_espnow_deinit();
}
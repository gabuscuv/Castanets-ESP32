#include "satellite_client_espnow.h"

#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_mac.h"

#include "ESPNOW_CONFIG.h"
#include "satellite_espnow_protocol.h"

static const char *TAG = "satellite_client";

static TaskHandle_t s_discovery_task = NULL;

static satellite_espnow_packet_callback_t s_recv_callback = NULL;

static uint8_t s_server_mac[ESP_NOW_ETH_ALEN];

static satellite_role_t s_requested_role = SATELLITE_ROLE_NONE;
static satellite_role_t s_role = SATELLITE_ROLE_NONE;

static volatile bool s_connected = false;

static TickType_t s_last_server_packet = 0;


/* -------------------------------------------------------------------------- */
/* Discovery                                                                   */
/* -------------------------------------------------------------------------- */

static void send_discovery(void)
{
    satellite_discover_packet_t packet = {
        .type = SATELLITE_MSG_DISCOVER,
        .requested_role = s_requested_role,
    };

    esp_err_t err = satellite_espnow_send_broadcast(
        (const uint8_t *)&packet,
        sizeof(packet));

    if (err != ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "Discovery send failed: %s",
            esp_err_to_name(err));
    }
}


static void satellite_client_discovery_task(void *arg)
{
    (void)arg;

    ESP_LOGI(
        TAG,
        "Discovery task started");

    while (true)
    {
        /*
         * Once connected, monitor the server.
         */
        if (s_connected)
        {
            TickType_t now = xTaskGetTickCount();

            const TickType_t timeout =
                pdMS_TO_TICKS(
                    CONFIG_ESPNOW_CONNECTION_TIMEOUT_MS);

            if ((now - s_last_server_packet) > timeout)
            {
                ESP_LOGW(
                    TAG,
                    "Server timeout; restarting discovery");

                s_connected = false;
                s_role = SATELLITE_ROLE_NONE;

                memset(
                    s_server_mac,
                    0,
                    sizeof(s_server_mac));
            }
        }

        /*
         * Broadcast discovery while disconnected.
         */
        if (!s_connected)
        {
            send_discovery();
        }

        vTaskDelay(
            pdMS_TO_TICKS(
                CONFIG_ESPNOW_DISCOVERY_INTERVAL_MS));
    }
}


/* -------------------------------------------------------------------------- */
/* RX                                                                         */
/* -------------------------------------------------------------------------- */

static esp_err_t satellite_client_rx(
    const uint8_t src_mac[ESP_NOW_ETH_ALEN],
    const uint8_t *data,
    uint16_t data_len)
{
    if (data == NULL || data_len == 0)
        return ESP_ERR_INVALID_ARG;

    const uint8_t type = data[0];

    if (type == SATELLITE_MSG_ASSIGN)
    {
        if (data_len < sizeof(satellite_assign_packet_t))
            return ESP_ERR_INVALID_SIZE;

        const satellite_assign_packet_t *assignment =
            (const satellite_assign_packet_t *)data;

        memcpy(
            s_server_mac,
            src_mac,
            ESP_NOW_ETH_ALEN);

        esp_err_t err =
            satellite_espnow_add_peer(s_server_mac);

        if (err != ESP_OK)
        {
            ESP_LOGE(
                TAG,
                "Failed to add server peer: %s",
                esp_err_to_name(err));

            return err;
        }

        s_role = assignment->role;
        s_connected = true;
        s_last_server_packet = xTaskGetTickCount();

        ESP_LOGI(
            TAG,
            "Connected to server " MACSTR " with role %u",
            MAC2STR(s_server_mac),
            s_role);

        return ESP_OK;
    }

    if (s_connected)
    {
        s_last_server_packet =
            xTaskGetTickCount();
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

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

esp_err_t satellite_client_espnow_init(
    satellite_espnow_packet_callback_t recv_cb,
    satellite_role_t requested_role)
{
    if (recv_cb == NULL)
        return ESP_ERR_INVALID_ARG;

    s_recv_callback = recv_cb;

    s_requested_role = requested_role;

    s_role = SATELLITE_ROLE_NONE;
    s_connected = false;

    memset(
        s_server_mac,
        0,
        sizeof(s_server_mac));

    esp_err_t err =
        satellite_espnow_init(
            satellite_client_rx);

    if (err != ESP_OK)
    {
        s_recv_callback = NULL;
        return err;
    }


    if (xTaskCreate(
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


esp_err_t satellite_client_espnow_send(
    const uint8_t *data,
    size_t data_len)
{
    if (!s_connected)
        return ESP_ERR_INVALID_STATE;

    if (data == NULL)
        return ESP_ERR_INVALID_ARG;

    if (data_len == 0 ||
        data_len > ESP_NOW_MAX_DATA_LEN)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    return satellite_espnow_send(
        s_server_mac,
        data,
        data_len);
}


bool satellite_client_espnow_is_connected(void)
{
    return s_connected;
}


satellite_role_t satellite_client_espnow_get_role(void)
{
    return s_role;
}


bool satellite_client_espnow_get_server_mac(
    uint8_t mac[ESP_NOW_ETH_ALEN])
{
    if (!s_connected || mac == NULL)
        return false;

    memcpy(
        mac,
        s_server_mac,
        ESP_NOW_ETH_ALEN);

    return true;
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
#include "satellite_client_protocol_discovery.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "portmacro.h"
#include "satellite_client_espnow_send.h"
#include "satellite_espnow_protocol.h"
#include "ESPNOW_CONFIG.h"

static const char *TAG = "satellite_client_protocol_discovery";


void send_discovery(void)
{
    satellite_discover_packet_t packet = {
        .type = SATELLITE_MSG_DISCOVER,
        .requested_role = SATELLITE_CONTROLLER_ROLE_UNKNOWN,
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

static volatile bool* s_connected_ptr;
static volatile TickType_t *s_last_server_packet_ptr;
static timeout_callback_t s_timeout_cb;

esp_err_t satellite_client_discovery_init(discovery_args args)
{
    esp_err_t err = ESP_OK;
  
    if (args.connected_ptr == NULL)
    {
        ESP_LOGE(TAG, "connected_ptr is NULL");
        err = ESP_ERR_INVALID_ARG;
    }

    if (args.last_server_packet_ptr == NULL)
    {
        ESP_LOGE(TAG, "last_server_packet_ptr is NULL");
        err = ESP_ERR_INVALID_ARG;
    }

    if (args.timeout_function_callback == NULL)
    {
        ESP_LOGE(TAG, "timeout_function_callback is NULL");
        err = ESP_ERR_INVALID_ARG;
    }

    if (err != ESP_OK){return err;}

    s_connected_ptr = args.connected_ptr;
    s_last_server_packet_ptr = args.last_server_packet_ptr;
    s_timeout_cb = args.timeout_function_callback;

    return err;
}

void satellite_client_discovery_task(void *arg)
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
        if (*s_connected_ptr)
        {
            TickType_t now = xTaskGetTickCount();

            const TickType_t timeout =
                pdMS_TO_TICKS(
                    CONFIG_ESPNOW_CONNECTION_TIMEOUT_MS);

            if ((now - *s_last_server_packet_ptr) > timeout)
            {
                ESP_LOGW(
                    TAG,
                    "Server timeout; restarting discovery");

                s_timeout_cb();
            }
        }

        /*
         * Broadcast discovery while disconnected.
         */
        if (!*s_connected_ptr)
        {
            send_discovery();
        }

        vTaskDelay(
            pdMS_TO_TICKS(
                CONFIG_ESPNOW_DISCOVERY_INTERVAL_MS));
    }
}

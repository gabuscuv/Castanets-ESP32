#include "satellite_espnow.h"

#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"

#include "ESPNOW_CONFIG.h"

static const char *TAG = "satellite_espnow";

#define SATELLITE_ESPNOW_MAX_PACKET ESP_NOW_MAX_DATA_LEN

static const uint8_t s_broadcast_mac[ESP_NOW_ETH_ALEN] = {
    0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF
};

typedef struct
{
    uint8_t src_mac[ESP_NOW_ETH_ALEN];
    uint8_t data[SATELLITE_ESPNOW_MAX_PACKET];
    uint16_t data_len;
} satellite_espnow_event_t;

static QueueHandle_t s_queue = NULL;
static TaskHandle_t s_task = NULL;

static satellite_espnow_packet_callback_t s_recv_callback = NULL;

static bool s_initialized = false;


/* -------------------------------------------------------------------------- */
/* Peer management                                                            */
/* -------------------------------------------------------------------------- */

bool satellite_espnow_peer_exists(
    const uint8_t mac[ESP_NOW_ETH_ALEN])
{
    return esp_now_is_peer_exist(mac);
}


esp_err_t satellite_espnow_add_peer(
    const uint8_t mac[ESP_NOW_ETH_ALEN])
{
    if (mac == NULL)
        return ESP_ERR_INVALID_ARG;

    if (esp_now_is_peer_exist(mac))
        return ESP_OK;

    esp_now_peer_info_t peer = {0};

    memcpy(
        peer.peer_addr,
        mac,
        ESP_NOW_ETH_ALEN);

    /*
     * Our WiFi is STA mode.
     */
    peer.ifidx = WIFI_IF_STA;

    /*
     * All devices are forced onto the configured ESP-NOW channel.
     */
    peer.channel = CONFIG_ESPNOW_CHANNEL;

    /*
     * Demo network: deliberately unencrypted.
     */
    peer.encrypt = false;

    esp_err_t err = esp_now_add_peer(&peer);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to add peer " MACSTR ": %s",
            MAC2STR(mac),
            esp_err_to_name(err));
    }

    return err;
}


/* -------------------------------------------------------------------------- */
/* RX callback                                                                */
/* -------------------------------------------------------------------------- */

static void satellite_espnow_recv_cb(
    const esp_now_recv_info_t *recv_info,
    const uint8_t *data,
    int data_len)
{
    if (s_queue == NULL)
        return;

    if (recv_info == NULL ||
        recv_info->src_addr == NULL ||
        data == NULL ||
        data_len <= 0 ||
        data_len > SATELLITE_ESPNOW_MAX_PACKET)
    {
        return;
    }

    satellite_espnow_event_t event = {
        .data_len = (uint16_t)data_len,
    };

    memcpy(
        event.src_mac,
        recv_info->src_addr,
        ESP_NOW_ETH_ALEN);

    memcpy(
        event.data,
        data,
        (size_t)data_len);

    /*
     * Never block the ESP-NOW callback.
     */
    if (xQueueSend(s_queue, &event, 0) != pdTRUE)
    {
        ESP_LOGW(
            TAG,
            "RX queue full; packet dropped");
    }
}


/* -------------------------------------------------------------------------- */
/* Worker                                                                     */
/* -------------------------------------------------------------------------- */

static void satellite_espnow_task(void *arg)
{
    (void)arg;

    satellite_espnow_event_t event;

    ESP_LOGI(TAG, "ESP-NOW worker started");

    while (true)
    {
        if (xQueueReceive(
                s_queue,
                &event,
                portMAX_DELAY) != pdTRUE)
        {
            continue;
        }

        if (s_recv_callback != NULL)
        {
            s_recv_callback(
                event.src_mac,
                event.data,
                event.data_len);
        }
    }
}


/* -------------------------------------------------------------------------- */
/* Initialization                                                             */
/* -------------------------------------------------------------------------- */

esp_err_t satellite_espnow_init(
    satellite_espnow_packet_callback_t recv_cb)
{
    if (recv_cb == NULL)
        return ESP_ERR_INVALID_ARG;

    if (s_initialized)
    {
        ESP_LOGW(TAG, "ESP-NOW already initialized");
        return ESP_OK;
    }

    s_queue = xQueueCreate(
        CONFIG_ESPNOW_QUEUE_SIZE,
        sizeof(satellite_espnow_event_t));

    if (s_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create RX queue");
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = esp_now_init();

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "esp_now_init failed: %s",
            esp_err_to_name(err));

        vQueueDelete(s_queue);
        s_queue = NULL;

        return err;
    }

    s_recv_callback = recv_cb;

    err = esp_now_register_recv_cb(
        satellite_espnow_recv_cb);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to register RX callback: %s",
            esp_err_to_name(err));

        esp_now_deinit();

        vQueueDelete(s_queue);
        s_queue = NULL;

        s_recv_callback = NULL;

        return err;
    }


    /*
     * Broadcast is used for discovery.
     *
     * Add FF:FF:FF:FF:FF:FF as an unencrypted peer.
     */
    err = satellite_espnow_add_peer(s_broadcast_mac);

    if (err != ESP_OK)
    {
        esp_now_deinit();

        vQueueDelete(s_queue);
        s_queue = NULL;

        s_recv_callback = NULL;

        return err;
    }


#if CONFIG_ESPNOW_ENABLE_POWER_SAVE

    err = esp_now_set_wake_window(
        CONFIG_ESPNOW_WAKE_WINDOW);

    if (err != ESP_OK)
    {
        esp_now_deinit();

        vQueueDelete(s_queue);
        s_queue = NULL;

        s_recv_callback = NULL;

        return err;
    }

    err = esp_wifi_connectionless_module_set_wake_interval(
        CONFIG_ESPNOW_WAKE_INTERVAL);

    if (err != ESP_OK)
    {
        esp_now_deinit();

        vQueueDelete(s_queue);
        s_queue = NULL;

        s_recv_callback = NULL;

        return err;
    }

#endif


    if (xTaskCreate(
            satellite_espnow_task,
            "sat_espnow",
            4096,
            NULL,
            4,
            &s_task) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create ESP-NOW task");

        esp_now_deinit();

        vQueueDelete(s_queue);
        s_queue = NULL;

        s_recv_callback = NULL;

        return ESP_ERR_NO_MEM;
    }

    s_initialized = true;

    ESP_LOGI(TAG, "ESP-NOW initialized");

    return ESP_OK;
}


/* -------------------------------------------------------------------------- */
/* TX                                                                         */
/* -------------------------------------------------------------------------- */

esp_err_t satellite_espnow_send(
    const uint8_t dest_mac[ESP_NOW_ETH_ALEN],
    const uint8_t *data,
    size_t data_len)
{
    if (!s_initialized)
        return ESP_ERR_INVALID_STATE;

    if (dest_mac == NULL || data == NULL)
        return ESP_ERR_INVALID_ARG;

    if (data_len == 0 ||
        data_len > SATELLITE_ESPNOW_MAX_PACKET)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    /*
     * Dynamically discovered peers are added lazily.
     *
     * Broadcast is already registered during init.
     */
    if (!satellite_espnow_peer_exists(dest_mac))
    {
        esp_err_t err =
            satellite_espnow_add_peer(dest_mac);

        if (err != ESP_OK)
            return err;
    }

    return esp_now_send(
        dest_mac,
        data,
        data_len);
}


esp_err_t satellite_espnow_send_broadcast(
    const uint8_t *data,
    size_t data_len)
{
    return satellite_espnow_send(
        s_broadcast_mac,
        data,
        data_len);
}


/* -------------------------------------------------------------------------- */
/* Deinitialization                                                           */
/* -------------------------------------------------------------------------- */

esp_err_t satellite_espnow_deinit(void)
{
    if (!s_initialized)
        return ESP_OK;

    s_initialized = false;

    if (s_task != NULL)
    {
        vTaskDelete(s_task);
        s_task = NULL;
    }

    esp_err_t err = esp_now_deinit();

    if (s_queue != NULL)
    {
        vQueueDelete(s_queue);
        s_queue = NULL;
    }

    s_recv_callback = NULL;

    if (err != ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "esp_now_deinit failed: %s",
            esp_err_to_name(err));
    }

    return err;
}
#include "satellite_client_espnow.h"

#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_mac.h"

#include "ESPNOW_CONFIG.h"

static const char *TAG = "satellite_client_espnow";

#define SATELLITE_CLIENT_ESPNOW_QUEUE_SIZE 6

static const uint8_t s_broadcast_mac[ESP_NOW_ETH_ALEN] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

typedef struct
{
    uint8_t src_mac[ESP_NOW_ETH_ALEN];
    uint8_t data[ESP_NOW_MAX_DATA_LEN];
    uint16_t data_len;
} satellite_client_espnow_event_t;

static QueueHandle_t s_espnow_queue = NULL;
static TaskHandle_t s_espnow_task = NULL;
static satellite_client_espnow_packet_callback_t s_recv_callback = NULL;
static bool s_initialized = false;

static bool is_broadcast(const uint8_t mac[ESP_NOW_ETH_ALEN])
{
    return memcmp(mac, s_broadcast_mac, ESP_NOW_ETH_ALEN) == 0;
}

static esp_err_t add_peer(
    const uint8_t mac[ESP_NOW_ETH_ALEN],
    bool encrypt)
{
    if (esp_now_is_peer_exist(mac))
        return ESP_OK;

    esp_now_peer_info_t peer = {0};

    memcpy(peer.peer_addr, mac, ESP_NOW_ETH_ALEN);
    peer.channel = CONFIG_ESPNOW_CHANNEL;
    peer.ifidx = ESP_IF_WIFI_AP;
    peer.encrypt = encrypt;

    if (encrypt)
    {
        memcpy(peer.lmk, CONFIG_ESPNOW_LMK, ESP_NOW_KEY_LEN);
    }

    esp_err_t err = esp_now_add_peer(&peer);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "Failed to add peer " MACSTR ": %s",
                 MAC2STR(mac),
                 esp_err_to_name(err));
    }

    return err;
}

/*
 * Runs in the Wi-Fi/ESP-NOW callback context.
 * Keep this function small: copy the packet and queue it.
 */
static void satellite_client_espnow_recv_cb(
    const esp_now_recv_info_t *recv_info,
    const uint8_t *data,
    int data_len)
{
    if (s_espnow_queue == NULL)
        return;

    if (recv_info == NULL ||
        recv_info->src_addr == NULL ||
        data == NULL ||
        data_len <= 0 ||
        data_len > ESP_NOW_MAX_DATA_LEN)
    {
        return;
    }

    satellite_client_espnow_event_t event = {
        .data_len = (uint16_t)data_len,
    };

    memcpy(event.src_mac,
           recv_info->src_addr,
           ESP_NOW_ETH_ALEN);

    memcpy(event.data,
           data,
           (size_t)data_len);

    /*
     * Never block the ESP-NOW callback.
     */
    if (xQueueSend(s_espnow_queue, &event, 0) != pdTRUE)
    {
        ESP_LOGW(TAG, "RX queue full; packet dropped");
    }
}

static void satellite_client_espnow_task(void *arg)
{
    (void)arg;

    satellite_client_espnow_event_t event;

    ESP_LOGI(TAG, "ESP-NOW worker started");

    while (true)
    {
        if (xQueueReceive(s_espnow_queue,
                          &event,
                          portMAX_DELAY) != pdTRUE)
        {
            continue;
        }

        if (s_recv_callback != NULL)
        {
            s_recv_callback(event.src_mac,
                            event.data,
                            event.data_len);
        }
    }
}

static esp_err_t create_queue(void)
{
    if (s_espnow_queue != NULL)
        return ESP_OK;

    s_espnow_queue = xQueueCreate(
        SATELLITE_CLIENT_ESPNOW_QUEUE_SIZE,
        sizeof(satellite_client_espnow_event_t));

    if (s_espnow_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create ESP-NOW RX queue");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

static esp_err_t init_stack(void)
{
    esp_err_t err = esp_now_init();

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "esp_now_init failed: %s",
                 esp_err_to_name(err));
        return err;
    }

    err = esp_now_register_recv_cb(
        satellite_client_espnow_recv_cb);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "esp_now_register_recv_cb failed: %s",
                 esp_err_to_name(err));
        esp_now_deinit();
        return err;
    }

#if CONFIG_ESPNOW_ENABLE_POWER_SAVE

    err = esp_now_set_wake_window(CONFIG_ESPNOW_WAKE_WINDOW);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "esp_now_set_wake_window failed: %s",
                 esp_err_to_name(err));
        esp_now_deinit();
        return err;
    }

    err = esp_wifi_connectionless_module_set_wake_interval(
        CONFIG_ESPNOW_WAKE_INTERVAL);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "Failed to set ESP-NOW wake interval: %s",
                 esp_err_to_name(err));
        esp_now_deinit();
        return err;
    }

#endif

    err = esp_now_set_pmk(
        (const uint8_t *)CONFIG_ESPNOW_PMK);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "esp_now_set_pmk failed: %s",
                 esp_err_to_name(err));
        esp_now_deinit();
        return err;
    }

    return ESP_OK;
}

esp_err_t satellite_client_espnow_init(
    satellite_client_espnow_packet_callback_t recv_cb)
{
    if (recv_cb == NULL)
        return ESP_ERR_INVALID_ARG;

    if (s_initialized)
    {
        ESP_LOGW(TAG, "ESP-NOW already initialized");
        return ESP_OK;
    }

    esp_err_t err = create_queue();
    if (err != ESP_OK)
        return err;

    err = init_stack();
    if (err != ESP_OK)
    {
        vQueueDelete(s_espnow_queue);
        s_espnow_queue = NULL;

        return err;
    }

    s_recv_callback = recv_cb;

    if (xTaskCreate(
            satellite_client_espnow_task,
            "sat_client_espnow",
            4096,
            NULL,
            4,
            &s_espnow_task) != pdPASS)
    {
        ESP_LOGE(
            TAG,
            "Failed to create ESP-NOW worker task");

        esp_now_deinit();

        vQueueDelete(s_espnow_queue);
        s_espnow_queue = NULL;

        return ESP_ERR_NO_MEM;
    }

    s_initialized = true;

    ESP_LOGI(TAG, "ESP-NOW initialized");

    return ESP_OK;
}

esp_err_t satellite_client_espnow_send( const uint8_t dest_mac[ESP_NOW_ETH_ALEN], const uint8_t *data, size_t data_len)
{
    if (!s_initialized) {return ESP_ERR_INVALID_STATE;}

    if (dest_mac == NULL || data == NULL) { return ESP_ERR_INVALID_ARG; }

    if (data_len == 0 || data_len > ESP_NOW_MAX_DATA_LEN) { return ESP_ERR_INVALID_SIZE; }

    const bool broadcast = is_broadcast(dest_mac);

    /*
     * Unicast requires a peer. Add it lazily on first transmission.
     */
    esp_err_t err = add_peer(dest_mac, !broadcast);

    if (err != ESP_OK) { return err; }

    err = esp_now_send(dest_mac, data, data_len);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "Failed to send packet to " MACSTR ": %s",
                 MAC2STR(dest_mac),
                 esp_err_to_name(err));
    }

    return err;
}

esp_err_t satellite_client_espnow_send_broadcast(
    const uint8_t *data,
    size_t data_len)
{
    return satellite_client_espnow_send(s_broadcast_mac, data, data_len);
}

esp_err_t satellite_client_espnow_deinit(void)
{
    if (!s_initialized) { return ESP_OK; }

    /*
     * Stop the worker before deleting its queue.
     */
    if (s_espnow_task != NULL)
    {
        vTaskDelete(s_espnow_task);
        s_espnow_task = NULL;
    }

    esp_err_t err = esp_now_deinit();

    if (s_espnow_queue != NULL)
    {
        vQueueDelete(s_espnow_queue);
        s_espnow_queue = NULL;
    }

    s_recv_callback = NULL;
    s_initialized = false;

    if (err != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "esp_now_deinit failed: %s",
                 esp_err_to_name(err));
    }

    return err;
}

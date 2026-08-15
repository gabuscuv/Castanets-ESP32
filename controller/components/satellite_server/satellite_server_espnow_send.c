// SHAMEFUL CODE
#include "satellite_server_espnow_send.h"
#include "esp_now.h"
#include <stdbool.h>
#include <string.h>

#include "esp_log.h"
#include "ESPNOW_CONFIG.h"

static const char *TAG = "satellite_espnow_send";

static const uint8_t s_broadcast_mac[ESP_NOW_ETH_ALEN] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static bool s_initialized = false;

static bool is_broadcast(
    const uint8_t mac[ESP_NOW_ETH_ALEN])
{
    return memcmp(
        mac,
        s_broadcast_mac,
        ESP_NOW_ETH_ALEN) == 0;
}

static esp_err_t add_peer(
    const uint8_t mac[ESP_NOW_ETH_ALEN],
    bool encrypt)
{
    if (esp_now_is_peer_exist(mac))
        return ESP_OK;

    esp_now_peer_info_t peer = {0};

    memcpy(
        peer.peer_addr,
        mac,
        ESP_NOW_ETH_ALEN);

    peer.channel = CONFIG_ESPNOW_CHANNEL;
    // peer.ifidx = ESPNOW_WIFI_IF; // TODO
    peer.encrypt = encrypt;

    if (encrypt)
    {
        memcpy(
            peer.lmk,
            CONFIG_ESPNOW_LMK,
            ESP_NOW_KEY_LEN);
    }

    esp_err_t err = esp_now_add_peer(&peer);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to add peer : %s",
           
            esp_err_to_name(err));
    }

    return err;
}

esp_err_t satellite_espnow_send_init(void)
{
    if (s_initialized)
        return ESP_OK;

    /*
     * ESP-NOW itself is initialized by satellite_server_espnow.c.
     * This module only owns its own bookkeeping.
     */
    s_initialized = true;

    return ESP_OK;
}

esp_err_t satellite_espnow_send(
    const uint8_t dest_mac[ESP_NOW_ETH_ALEN],
    const uint8_t *data,
    size_t data_len)
{
    if (!s_initialized)
        return ESP_ERR_INVALID_STATE;

    if (dest_mac == NULL || data == NULL)
        return ESP_ERR_INVALID_ARG;

    if (data_len == 0 || data_len > ESP_NOW_MAX_DATA_LEN)
        return ESP_ERR_INVALID_SIZE;

    const bool broadcast = is_broadcast(dest_mac);

    /*
     * Unicast packets require a peer entry.
     * Peers are added lazily when the application sends to them.
     */
    esp_err_t err = add_peer(dest_mac, !broadcast);

    if (err != ESP_OK)
        return err;

    err = esp_now_send(
        dest_mac,
        data,
        data_len);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to send packet to : %s",
            
            esp_err_to_name(err));
    }

    return err;
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

esp_err_t satellite_espnow_send_deinit(void)
{
    if (!s_initialized)
        return ESP_OK;

    /*
     * Do NOT call esp_now_deinit() here.
     * satellite_server_espnow.c owns the ESP-NOW lifecycle.
     */
    s_initialized = false;

    return ESP_OK;
}

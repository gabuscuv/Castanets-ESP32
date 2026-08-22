#include "satellite_client_espnow_send.h"
#include "ESPNOW_CONFIG.h"
#include "esp_now.h"
#include "satellite_espnow_protocol.h"

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

esp_err_t satellite_espnow_send(
    const uint8_t dest_mac[ESP_NOW_ETH_ALEN],
    const uint8_t *data,
    size_t data_len)
{
    if (dest_mac == NULL || data == NULL)
        return ESP_ERR_INVALID_ARG;

    if (data_len == 0 ||
        data_len > SATELLITE_ESPNOW_MAX_PACKET)
    {
        return ESP_ERR_INVALID_SIZE;
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
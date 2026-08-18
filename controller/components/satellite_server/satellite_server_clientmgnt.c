#include "satellite_server_clientmgnt.h"

#include <stdint.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_mac.h"

#include "satellite_espnow_protocol.h"

static const char *TAG = "satellite_server_clientmgnt";

typedef struct
{
    bool used;

    uint8_t mac[ESP_NOW_ETH_ALEN];

    satellite_role_t role;

} satellite_client_entry_t;


static satellite_client_entry_t s_clients[
    SATELLITE_SERVER_MAX_CLIENTS
];


static bool s_initialized = false;

esp_err_t satellite_server_clientmgnt_init() {
        memset(
        s_clients,
        0,
        sizeof(s_clients));

    s_initialized = true;
  return ESP_OK;
}

esp_err_t satellite_server_clientmgnt_deinit() {
    memset(
        s_clients,
        0,
        sizeof(s_clients));

    s_initialized = false;
    return ESP_OK;
}

static satellite_client_entry_t *find_client(
    const uint8_t mac[ESP_NOW_ETH_ALEN])
{
    if (mac == NULL)
        return NULL;

    for (size_t i = 0; i < SATELLITE_SERVER_MAX_CLIENTS; ++i)
    {
        if (!s_clients[i].used)
            continue;

        if (memcmp(
                s_clients[i].mac,
                mac,
                ESP_NOW_ETH_ALEN) == 0)
        {
            return &s_clients[i];
        }
    }

    return NULL;
}


static satellite_client_entry_t *find_free_client(void)
{
    for (size_t i = 0; i < SATELLITE_SERVER_MAX_CLIENTS; ++i)
    {
        if (!s_clients[i].used)
            return &s_clients[i];
    }

    return NULL;
}


esp_err_t satellite_server_clientmgnt_register_client(
    const uint8_t mac[ESP_NOW_ETH_ALEN],
    satellite_role_t role)
{
    if (mac == NULL)
        return ESP_ERR_INVALID_ARG;

    satellite_client_entry_t *client =
        find_client(mac);

    /*
     * Client already known.
     *
     * Update its role in case it requested something different.
     */
    if (client != NULL)
    {
        client->role = role;

        ESP_LOGI(
            TAG,
            "Updated client " MACSTR " role=%u",
            MAC2STR(mac),
            role);

        return ESP_OK;
    }

    client = find_free_client();

    if (client == NULL)
    {
        ESP_LOGW(
            TAG,
            "Client table full; cannot register " MACSTR,
            MAC2STR(mac));

        return ESP_ERR_NO_MEM;
    }

    memset(client, 0, sizeof(*client));

    client->used = true;
    client->role = role;

    memcpy(
        client->mac,
        mac,
        ESP_NOW_ETH_ALEN);

    ESP_LOGI(
        TAG,
        "Registered client " MACSTR " role=%u",
        MAC2STR(mac),
        role);

    return ESP_OK;
}


bool satellite_server_clientmgnt_get_client_role(
    const uint8_t mac[ESP_NOW_ETH_ALEN],
    satellite_role_t *role)
{
    satellite_client_entry_t *client =
        find_client(mac);

    if (client == NULL)
        return false;

    if (role != NULL)
        *role = client->role;

    return true;
}

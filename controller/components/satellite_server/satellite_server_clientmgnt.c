#include "satellite_server_clientmgnt.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_mac.h"

#include "satellite_espnow_protocol.h"

static const char *TAG = "satellite_server_clientmgnt";
static bool s_initialized = false;
typedef struct
{
    bool used;

    uint8_t mac[ESP_NOW_ETH_ALEN];

    satellite_role_t role;

} satellite_client_entry_t;


static satellite_client_entry_t s_clients[SATELLITE_SERVER_MAX_CLIENTS];

size_t satellite_server_clientmgnt_get_clients(
    satellite_client_info_t *clients,
    size_t max_clients)
{
    if (clients == NULL || max_clients == 0)
        return 0;

    size_t count = 0;

    for (size_t i = 0;
         i < SATELLITE_SERVER_MAX_CLIENTS && count < max_clients;
         ++i)
    {
        if (!s_clients[i].used)
            continue;

        memcpy(
            clients[count].mac,
            s_clients[i].mac,
            ESP_NOW_ETH_ALEN);

        clients[count].role = s_clients[i].role;

        ++count;
    }

    return count;
}
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

satellite_role_t satellite_server_clientmgnt_get_role_available(void)
{
    bool left_occupied = false;
    bool right_occupied = false;

    for (size_t i = 0; i < SATELLITE_SERVER_MAX_CLIENTS; ++i)
    {
        if (!s_clients[i].used)
            continue;

        switch (s_clients[i].role)
        {
        case SATELLITE_CONTROLLER_ROLE_LEFT:
            left_occupied = true;
            break;

        case SATELLITE_CONTROLLER_ROLE_RIGHT:
            right_occupied = true;
            break;

        default:
            break;
        }

        if (left_occupied && right_occupied)
            break;
    }

    if (!left_occupied)
    {
        return SATELLITE_CONTROLLER_ROLE_LEFT;
    }
    if (!right_occupied)
    {
        return SATELLITE_CONTROLLER_ROLE_RIGHT;
    }
    return SATELLITE_CONTROLLER_ROLE_UNKNOWN;
}

esp_err_t satellite_server_clientmgnt_register_client(
    const uint8_t mac[ESP_NOW_ETH_ALEN],
    satellite_role_t role)
{
    if (!s_initialized) {return ESP_ERR_INVALID_STATE;}

    if (mac == NULL) {return ESP_ERR_INVALID_ARG;}

    if (role != SATELLITE_CONTROLLER_ROLE_LEFT &&
        role != SATELLITE_CONTROLLER_ROLE_RIGHT)
    {
        return ESP_ERR_INVALID_ARG;
    }

    satellite_client_entry_t *client = find_client(mac);

    /*
     * Client already known.
     *
     * Update its role in case it requested something different.
     */
    if (client != NULL)
    {

        if (client->role == role)
            return ESP_OK;

        for (size_t i = 0; i < SATELLITE_SERVER_MAX_CLIENTS; ++i)
        {
            if (!s_clients[i].used)
                continue;

            if (&s_clients[i] == client)
                continue;

            if (s_clients[i].role == role)
                return ESP_ERR_INVALID_STATE;
        }

        client->role = role;

        ESP_LOGI(
            TAG,
            "Updated client " MACSTR " role=%u",
            MAC2STR(mac),
            role);

        return ESP_OK;
    }

    satellite_client_entry_t *free_client = find_free_client();

    if (free_client == NULL)
    {
        ESP_LOGW(
            TAG,
            "Client table full; cannot register " MACSTR,
            MAC2STR(mac));

        return ESP_ERR_NO_MEM;
    }

    for (size_t i = 0; i < SATELLITE_SERVER_MAX_CLIENTS; ++i)
    {
        if (!s_clients[i].used){continue;}

        if (s_clients[i].role == role){return ESP_ERR_INVALID_STATE;}
    }

    free_client->used = true;
    free_client->role = role;

    memcpy(
        free_client->mac,
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

#include "satellite_server_espnow.h"

#include <string.h>

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "satellite_espnow_protocol.h"

#define SATELLITE_SERVER_MAX_CLIENTS 8

static const char *TAG = "satellite_server_espnow";

typedef struct
{
    bool used;

    uint8_t mac[ESP_NOW_ETH_ALEN];

    satellite_role_t role;
} satellite_server_client_t;


static satellite_server_client_t s_clients[
    SATELLITE_SERVER_MAX_CLIENTS
];

static satellite_espnow_packet_callback_t s_recv_callback = NULL;

static satellite_server_role_assign_callback_t
    s_role_assign_callback = NULL;


/* -------------------------------------------------------------------------- */
/* Client table                                                               */
/* -------------------------------------------------------------------------- */

static satellite_server_client_t *find_client(
    const uint8_t mac[ESP_NOW_ETH_ALEN])
{
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


static satellite_server_client_t *create_client(
    const uint8_t mac[ESP_NOW_ETH_ALEN],
    satellite_role_t role)
{
    for (size_t i = 0; i < SATELLITE_SERVER_MAX_CLIENTS; ++i)
    {
        if (s_clients[i].used)
            continue;

        s_clients[i].used = true;
        s_clients[i].role = role;

        memcpy(
            s_clients[i].mac,
            mac,
            ESP_NOW_ETH_ALEN);

        return &s_clients[i];
    }

    return NULL;
}


/* -------------------------------------------------------------------------- */
/* Discovery                                                                   */
/* -------------------------------------------------------------------------- */

static void handle_discovery(
    const uint8_t client_mac[ESP_NOW_ETH_ALEN],
    const uint8_t *data,
    uint16_t data_len)
{
    if (data_len < sizeof(satellite_discover_packet_t))
        return;

    const satellite_discover_packet_t *request =
        (const satellite_discover_packet_t *)data;

    satellite_server_client_t *client =
        find_client(client_mac);

    satellite_role_t role;

    /*
     * Existing client:
     *
     * Don't assign a different role every time it broadcasts.
     */
    if (client != NULL)
    {
        role = client->role;
    }
    else
    {
        role = SATELLITE_ROLE_NONE;

        if (s_role_assign_callback != NULL)
        {
            role = s_role_assign_callback(
                client_mac,
                request->requested_role);
        }

        if (role == SATELLITE_ROLE_NONE)
        {
            ESP_LOGW(
                TAG,
                "Rejected client " MACSTR,
                MAC2STR(client_mac));

            return;
        }

        client = create_client(
            client_mac,
            role);

        if (client == NULL)
        {
            ESP_LOGE(
                TAG,
                "Client table full");

            return;
        }

        ESP_LOGI(
            TAG,
            "New client " MACSTR " assigned role %u",
            MAC2STR(client_mac),
            role);
    }


    /*
     * Register the client for future unicast.
     */
    esp_err_t err =
        satellite_espnow_add_peer(client_mac);

    if (err != ESP_OK)
        return;


    /*
     * Get our own STA MAC.
     */
    uint8_t server_mac[ESP_NOW_ETH_ALEN];

    err = esp_wifi_get_mac(
        WIFI_IF_STA,
        server_mac);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to get server MAC: %s",
            esp_err_to_name(err));

        return;
    }


    satellite_assign_packet_t response = {
        .type = SATELLITE_MSG_ASSIGN,
        .role = role,
    };

    memcpy(
        response.server_mac,
        server_mac,
        ESP_NOW_ETH_ALEN);


    err = satellite_espnow_send(
        client_mac,
        (const uint8_t *)&response,
        sizeof(response));

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to send assignment to " MACSTR ": %s",
            MAC2STR(client_mac),
            esp_err_to_name(err));
    }
}


/* -------------------------------------------------------------------------- */
/* RX                                                                         */
/* -------------------------------------------------------------------------- */

static esp_err_t satellite_server_rx(
    const uint8_t client_mac[ESP_NOW_ETH_ALEN],
    const uint8_t *data,
    uint16_t data_len)
{
    if (data == NULL || data_len == 0)
        return ESP_ERR_INVALID_ARG;

    const uint8_t type = data[0];

    if (type == SATELLITE_MSG_DISCOVER)
    {
        handle_discovery(
            client_mac,
            data,
            data_len);

        return ESP_OK;
    }

    /*
     * Everything else is application data.
     */
    if (s_recv_callback != NULL)
    {
        return s_recv_callback(
            client_mac,
            data,
            data_len);
    }

    return ESP_OK;
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

esp_err_t satellite_server_espnow_init(
    satellite_espnow_packet_callback_t recv_cb,
    satellite_server_role_assign_callback_t role_assign_cb)
{
    if (recv_cb == NULL)
        return ESP_ERR_INVALID_ARG;

    memset(
        s_clients,
        0,
        sizeof(s_clients));

    s_recv_callback = recv_cb;
    s_role_assign_callback = role_assign_cb;

    return satellite_espnow_init(
        satellite_server_rx);
}


bool satellite_server_espnow_get_role(
    const uint8_t client_mac[ESP_NOW_ETH_ALEN],
    satellite_role_t *role)
{
    if (client_mac == NULL || role == NULL)
        return false;

    satellite_server_client_t *client =
        find_client(client_mac);

    if (client == NULL)
        return false;

    *role = client->role;

    return true;
}


esp_err_t satellite_server_espnow_deinit(void)
{
    memset(
        s_clients,
        0,
        sizeof(s_clients));

    s_recv_callback = NULL;
    s_role_assign_callback = NULL;

    return satellite_espnow_deinit();
}
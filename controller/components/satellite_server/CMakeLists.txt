idf_component_register(
    SRCS
        "satellite_server_espnow.c"
        "satellite_server_espnow_send.c"
        "satellite_server_wifi_init.c"
        "satellite_server_protocol.c"
        "satellite_server_init.c"
    INCLUDE_DIRS
        "include"
    
    PRIV_REQUIRES json esp_event esp_netif esp_wifi
)
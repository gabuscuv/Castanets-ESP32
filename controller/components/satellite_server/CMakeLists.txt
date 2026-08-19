idf_component_register(
    SRCS
        "satellite_espnow.c"
        "satellite_server_espnow_send.c"
        "satellite_server_wifi.c"
        "satellite_server_clientmgnt.c"
        "satellite_server_protocol.c"
        "satellite_server_protocol_parsers.c"
        "satellite_server_protocol_discovery.c"
        "satellite_server.c"
    INCLUDE_DIRS
        "include"
    
    PRIV_REQUIRES json esp_event esp_netif esp_wifi nvs_flash
)
idf_component_register(
    SRCS
        "satellite_client_wifi.c"
        "satellite_espnow.c"
        "satellite_client_espnow_send.c" 
        "satellite_client_espnow.c"
        "satellite_client_protocol_discovery.c" 
        "satellite_client_protocol_parser.c" 
        "satellite_client_protocol.c"
        "satellite_client.c"
        
    INCLUDE_DIRS
        "include"
    
    PRIV_REQUIRES
        nvs_flash 
        esp_event 
        esp_netif 
        esp_wifi 
        json

    
)
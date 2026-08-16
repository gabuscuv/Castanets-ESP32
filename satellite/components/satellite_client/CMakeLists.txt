idf_component_register(
    SRCS
        "satellite_client_wifi.c"
        "satellite_client_espnow.c"
        "satellite_client_protocol.c" 
        "satellite_client.c"
        
    INCLUDE_DIRS
        "include"
    
    PRIV_REQUIRES 
        esp_event 
        esp_netif 
        esp_wifi 
        json

    
)
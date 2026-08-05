idf_component_register(
    SRCS
        "espnow_server_init.c"

    INCLUDE_DIRS
        "include"
    
    PRIV_REQUIRES esp_event esp_netif esp_wifi
)
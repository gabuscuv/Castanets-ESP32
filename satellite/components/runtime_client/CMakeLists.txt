idf_component_register(
    SRCS
        "runtime_client.c"

    INCLUDE_DIRS
        "include"
    
    REQUIRES esp_timer satellite_client piezo_controller led_controller

)
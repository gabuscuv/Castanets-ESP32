idf_component_register(
    SRCS
        "piezo_init.c"

    INCLUDE_DIRS
        "include"

    PRIV_REQUIRES esp_driver_gpio
    
)
idf_component_register(
    SRCS
        "piezocontroller.c"

    INCLUDE_DIRS
        "include"

    PRIV_REQUIRES esp_driver_gpio
    
)
idf_component_register(
    SRCS
        "runtime_client_init.c"

    INCLUDE_DIRS
        "include"
    
    REQUIRES satellite_client piezo_controller

)
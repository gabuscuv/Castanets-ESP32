idf_component_register(
    SRCS
        "runtime_client.c"

    INCLUDE_DIRS
        "include"
    
    REQUIRES satellite_client piezo_controller

)
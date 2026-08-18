idf_component_register(
    SRCS
        "runtime_server.c"

    INCLUDE_DIRS
        "include"
    
    REQUIRES pc_comm pc_presentation satellite_server joycon_controller 
)
idf_component_register(
    SRCS
        serial/usbdevice_serial_commstates.c
        serial/usbdevice_serial_rx.c
        serial/usbdevice_serial_send.c
        serial/serial_init.c
        pc_comm_protocol_parse.c  
        pc_comm_protocol.c
        pc_comm.c

    INCLUDE_DIRS
        "include"
    
    REQUIRES
        esp_common
        freertos
        json
        tinyusb
        pc_presentation
)
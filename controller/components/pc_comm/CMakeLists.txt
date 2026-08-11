idf_component_register(
    SRCS
       pc_comm_init.c
       serial/serial_init.c
       serial/usbdevice_serial_from.c
       serial/usbdevice_serial_to.c
       serial/usbdevice_serial_commstates.c

    INCLUDE_DIRS
        "include"
    
    REQUIRES
        esp_common
        freertos
        json
        tinyusb
)
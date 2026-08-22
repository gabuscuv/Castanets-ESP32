#pragma once
#include "freertos/FreeRTOS.h"

typedef void(*timeout_callback_t)(void);

typedef struct {
    volatile bool *connected_ptr;
    volatile TickType_t *last_server_packet_ptr;
    timeout_callback_t timeout_function_callback;
} discovery_args;

esp_err_t satellite_client_discovery_init(discovery_args);
void satellite_client_discovery_task(void*);
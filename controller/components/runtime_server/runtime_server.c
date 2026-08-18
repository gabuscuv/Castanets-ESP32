#include "runtime_server.h"
#include "esp_err.h"
#include "esp_log.h"
#include "pc_comm.h"
#include "satellite_server.h"

static const char *TAG = "runtime_server";

int runtime_server_init() {
    ESP_LOGI(TAG, "Initializing PC COMM");
    esp_err_t err;
    // err = pc_comm_init();
    // if (err != ESP_OK){ return 1;}

    err = satellite_server_init();
    if (err != ESP_OK){ return 1;}

    
  return  0;
}
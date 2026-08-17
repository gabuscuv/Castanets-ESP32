#include "runtime_server.h"
#include "esp_log.h"
#include "pc_comm_init.h"
#include "satellite_server.h"

static const char *TAG = "runtime_server";

int runtime_server_init() {
    ESP_LOGI(TAG, "Initializing PC COMM");
  
    if (pc_comm_init())
    {
        return 1;
    }

     if (satellite_server_init())
    {
        return 1;
    }
    
  return  0;
}
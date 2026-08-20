#include "runtime_client.h"
#include "esp_err.h"
#include "esp_log.h"
#include "piezocontroller.h"
#include "satellite_client.h"
#include "ledcontroller.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "runtime_client";

static uint32_t s_time;

static esp_err_t runtime_client_callback(satellite_message_runtime_t msg)
{
  switch (msg.type) {

    case CONTROLLER_CMD_SET_GAME_TIME:
        s_time = msg.time;
        break;
    case CONTROLLER_CMD_RESET_TIMEHUB:
        s_time = 0;
        break;
    case CONTROLLER_ACK_ROLE:
    case CONTROLLER_CMD_START_SONG:
    
    case CONTROLLER_CMD_REQUEST_STATUS:
      break;
    }
    return ESP_OK;
}

static void runtime_piezo_callback(void)
{
    satellite_client_push_click(s_time);
}

#ifdef PIEZO_MOCK
static void piezo_mock(void *pvParameter)
{
    (void)pvParameter;

    while (true) {
        ESP_LOGI(TAG, "[PIEZO_MOCK] Sending CALLBACK");
        runtime_piezo_callback();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#endif

esp_err_t runtime_client_init(void)
{
    esp_err_t err;
    ESP_LOGI(TAG, "Initializing Client Runtime");

    ESP_LOGI(TAG, "Initializing LED Controller");
    err = ledcontroller_init();
    if (err != ESP_OK){return err;}

    ESP_LOGI(TAG, "Starting Satellite Client");
    err = satellite_client_init(runtime_client_callback);
    if (err != ESP_OK){return err;}

    ESP_LOGI(TAG, "Intializing Piezo Controller");
    err = piezocontroller_init(runtime_piezo_callback);
    if (err != ESP_OK){return err;}

#ifdef PIEZO_MOCK
    ESP_LOGI(TAG, "[PIEZO_MOCK] Intializing Piezo Mock Task");
    xTaskCreate(
        piezo_mock,
        "piezo_mock_task",
        2048,
        NULL,
        1,
        NULL);
#endif
    
    ledcontroller_blink(0,100);
    
    return ESP_OK;
}
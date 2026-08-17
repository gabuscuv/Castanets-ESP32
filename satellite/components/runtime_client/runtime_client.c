#include "runtime_client.h"
#include "esp_log.h"
#include "piezocontroller.h"
#include "satellite_client.h"
#include "ledcontroller.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "runtime_client";

static uint32_t s_time;

static void runtime_client_time_callback(uint32_t time)
{
    s_time = time; 
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
    err = satellite_client_init(runtime_client_time_callback);
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
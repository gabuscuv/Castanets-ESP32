#include "runtime_client.h"
#include "piezocontroller.h"
#include "satellite_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

    while (true)
    {
        runtime_piezo_callback();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#endif

esp_err_t runtime_client_init(void)
{
    esp_err_t err;

    err = satellite_client_init(runtime_client_time_callback);

    if (err != ESP_OK){return err;}

    err = piezocontroller_init(runtime_piezo_callback);

    if (err != ESP_OK){return err;}

#ifdef PIEZO_MOCK
    xTaskCreate(
        piezo_mock,
        "piezo_mock_task",
        2048,
        NULL,
        1,
        NULL);
#endif

    return ESP_OK;
}
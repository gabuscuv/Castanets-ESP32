#include "runtime_client.h"
#include "esp_err.h"
#include "esp_log.h"
#include <inttypes.h>
#include "esp_timer.h"
#include "piezocontroller.h"
#include "satellite_client.h"
#include "ledcontroller.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "runtime_client";

/*
 * Game time at the moment represented by s_game_time_start_us.
 *
 * Example:
 *   s_game_time_start_us = esp_timer_get_time() - 5000000
 *
 * means game time is currently 5000 ms.
 */
static int64_t s_game_time_start_us;

static void runtime_client_reset_game_time(void)
{
    s_game_time_start_us = esp_timer_get_time();
}

static void runtime_client_set_game_time(uint32_t game_time_ms)
{
    s_game_time_start_us = esp_timer_get_time() - ((int64_t)game_time_ms * 1000);
}

static uint32_t runtime_client_get_game_time(void)
{
    const int64_t elapsed_us = esp_timer_get_time() - s_game_time_start_us;

    if (elapsed_us <= 0){return 0;}

    return (uint32_t)(elapsed_us / 1000);
}

static esp_err_t runtime_client_callback(satellite_message_runtime_t msg)
{
    switch (msg.type) {

    case CONTROLLER_CMD_SET_GAME_TIME:
        runtime_client_set_game_time(msg.time);
        ESP_LOGI(
            TAG,
            "Game time synchronized: %" PRIu32 " ms",
            msg.time
        );
        break;

    case CONTROLLER_CMD_RESET_TIMEHUB:
        runtime_client_reset_game_time();

        ESP_LOGI(TAG, "Game time reset");
        break;

    case CONTROLLER_CMD_BLINK_SATELLITE:
        ledcontroller_blink(100, 900);
        break;

    case CONTROLLER_ACK_ROLE:
    case CONTROLLER_CMD_START_SONG:
    case CONTROLLER_CMD_REQUEST_STATUS:
        break;
    default:
        ESP_LOGW(TAG, "Unhandled runtime message type: %d", msg.type);
        break;
    }

    return ESP_OK;
}

static void runtime_piezo_callback(void)
{
    satellite_client_push_click(runtime_client_get_game_time()
);
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
    runtime_client_reset_game_time();

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
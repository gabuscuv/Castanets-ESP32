#include "satellite_server_espnow.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"

#include "ESPNOW_CONFIG.h"

static const char *TAG = "satellite_espnow";

#define SATELLITE_ESPNOW_QUEUE_SIZE   6
#define SATELLITE_ESPNOW_MAX_PACKET   ESP_NOW_MAX_DATA_LEN

static QueueHandle_t s_espnow_queue = NULL;
static TaskHandle_t s_espnow_task = NULL;
static satellite_espnow_packet_callback_t s_rcv_callback;

typedef struct
{
    uint8_t src_mac[ESP_NOW_ETH_ALEN];
    uint8_t data[SATELLITE_ESPNOW_MAX_PACKET];
    uint16_t data_len;
} satellite_espnow_event_t;


/*
 * Application-level packet processing.
 *
 * Keep JSON parsing here, rather than in the ESP-NOW callback.
 *
 * Replace the body of this function with your actual JSON parser /
 * satellite state update.
 */
static void satellite_espnow_process_packet(
    const satellite_espnow_event_t *event)
{
    s_rcv_callback(event->src_mac,event->data, event->data_len);
}


/*
 * ESP-NOW receive callback.
 *
 * This function intentionally does as little work as possible:
 *
 *   1. Validate the packet size.
 *   2. Copy the packet.
 *   3. Put it into the worker queue.
 *
 * Do NOT perform JSON parsing here.
 */
static void satellite_espnow_recv_cb(
    const esp_now_recv_info_t *recv_info,
    const uint8_t *data,
    int data_len)
{
    if (s_espnow_queue == NULL)
        return;

    if (recv_info == NULL || recv_info->src_addr == NULL)
        return;

    if (data == NULL || data_len <= 0)
        return;

    if (data_len > SATELLITE_ESPNOW_MAX_PACKET)
    {
        ESP_LOGW(
            TAG,
            "Dropping oversized packet: %d bytes",
            data_len);
        return;
    }

    satellite_espnow_event_t event = {
        .data_len = (uint16_t)data_len,
    };

    memcpy(
        event.src_mac,
        recv_info->src_addr,
        ESP_NOW_ETH_ALEN);

    memcpy(
        event.data,
        data,
        (size_t)data_len);

    /*
     * Never block the ESP-NOW callback waiting for the worker.
     *
     * If the queue is full, dropping the newest packet is preferable
     * to blocking the Wi-Fi callback.
     */
    if (xQueueSend(s_espnow_queue, &event, 0) != pdTRUE)
    {
        ESP_LOGW(TAG, "ESP-NOW receive queue full; packet dropped");
    }
}


static void satellite_espnow_task(void *arg)
{
    (void)arg;

    satellite_espnow_event_t event;

    ESP_LOGI(TAG, "ESP-NOW worker started");

    while (true)
    {
        if (xQueueReceive(
                s_espnow_queue,
                &event,
                portMAX_DELAY) != pdTRUE)
        {
            continue;
        }

        satellite_espnow_process_packet(&event);
    }
}


static esp_err_t satellite_espnow_create_queue(void)
{
    if (s_espnow_queue != NULL)
        return ESP_OK;

    s_espnow_queue = xQueueCreate(
        SATELLITE_ESPNOW_QUEUE_SIZE,
        sizeof(satellite_espnow_event_t));

    if (s_espnow_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create ESP-NOW queue");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}


static esp_err_t satellite_espnow_init_stack(void)
{
    esp_err_t err;

    err = esp_now_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "esp_now_init failed: %s",
            esp_err_to_name(err));
        return err;
    }

    err = esp_now_register_recv_cb(satellite_espnow_recv_cb);
    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "esp_now_register_recv_cb failed: %s",
            esp_err_to_name(err));

        esp_now_deinit();
        return err;
    }

#if CONFIG_ESPNOW_ENABLE_POWER_SAVE

    err = esp_now_set_wake_window(CONFIG_ESPNOW_WAKE_WINDOW);
    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "esp_now_set_wake_window failed: %s",
            esp_err_to_name(err));

        esp_now_deinit();
        return err;
    }

    err = esp_wifi_connectionless_module_set_wake_interval(
        CONFIG_ESPNOW_WAKE_INTERVAL);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to set ESP-NOW wake interval: %s",
            esp_err_to_name(err));

        esp_now_deinit();
        return err;
    }

#endif

    err = esp_now_set_pmk((const uint8_t *)CONFIG_ESPNOW_PMK);
    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "esp_now_set_pmk failed: %s",
            esp_err_to_name(err));

        esp_now_deinit();
        return err;
    }

    return ESP_OK;
}


esp_err_t satellite_espnow_init(satellite_espnow_packet_callback_t recv_cb)
{
    esp_err_t err;

    if (s_espnow_task != NULL)
    {
        ESP_LOGW(TAG, "ESP-NOW already initialized");
        return ESP_OK;
    }

    err = satellite_espnow_create_queue();
    if (err != ESP_OK)
        return err;

    err = satellite_espnow_init_stack();
    if (err != ESP_OK)
    {
        vQueueDelete(s_espnow_queue);
        s_espnow_queue = NULL;
        return err;
    }

    s_rcv_callback = recv_cb;

    BaseType_t task_result = xTaskCreate(
        satellite_espnow_task,
        "sat_espnow",
        4096,
        NULL,
        4,
        &s_espnow_task);

    if (task_result != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create ESP-NOW worker task");

        esp_now_deinit();

        vQueueDelete(s_espnow_queue);
        s_espnow_queue = NULL;

        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "ESP-NOW initialized");

    return ESP_OK;
}


esp_err_t satellite_espnow_deinit(void)
{
    if (s_espnow_task != NULL)
    {
        vTaskDelete(s_espnow_task);
        s_espnow_task = NULL;
    }

    if (s_espnow_queue != NULL)
    {
        vQueueDelete(s_espnow_queue);
        s_espnow_queue = NULL;
    }

    esp_err_t err = esp_now_deinit();

    if (err != ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "esp_now_deinit failed: %s",
            esp_err_to_name(err));
    }

    return err;
}

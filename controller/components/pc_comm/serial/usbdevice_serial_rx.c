#include "usbdevice_serial_rx.h"

#include <string.h>

#include "tinyusb_cdc_acm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "../pc_comm_protocol.h"

static const char *TAG = "USBDEVICE_SERIAL_RX";

#define CONFIG_PC_COMM_QUEUE_SIZE 5
#define CONFIG_PC_COMM_TASK_STACK_SIZE 5
#define CONFIG_PC_COMM_TASK_PRIORITY 1

static uint8_t rx_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE];

static QueueHandle_t app_queue;

static void usbdevice_serial_rx_task(void *arg)
{
    app_message_t msg;

    while (1)
    {
        if (xQueueReceive(app_queue, &msg, portMAX_DELAY) == pdTRUE)
        {
            esp_err_t ret = pccomm_protocol_handle(
                msg.itf,
                msg.buf,
                msg.buf_len);

            if (ret != ESP_OK)
            {
                ESP_LOGW(
                    TAG,
                    "Protocol handler failed: %s",
                    esp_err_to_name(ret));
            }
        }
    }
}

void usbdevice_serial_rx_callback(
    int itf,
    cdcacm_event_t *event)
{
    size_t rx_size = 0;

    esp_err_t ret = tinyusb_cdcacm_read(
        itf,
        rx_buf,
        CONFIG_TINYUSB_CDC_RX_BUFSIZE,
        &rx_size);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Read Error: %s", esp_err_to_name(ret));
        return;
    }

    if (rx_size == 0)
        return;

    app_message_t tx_msg = {
        .buf_len = rx_size,
        .itf = itf,
    };

    memcpy(tx_msg.buf, rx_buf, rx_size);

    if (xQueueSend(app_queue, &tx_msg, 0) != pdTRUE)
    {
        ESP_LOGW(TAG, "Application queue full, dropping packet");
    }
}

esp_err_t usbdevice_serial_rx_init(void)
{
    app_queue = xQueueCreate(
        CONFIG_PC_COMM_QUEUE_SIZE,
        sizeof(app_message_t));

    if (app_queue == NULL)
        return ESP_ERR_NO_MEM;

    BaseType_t ret = xTaskCreate(
        usbdevice_serial_rx_task,
        "pccomm_rx",
        CONFIG_PC_COMM_TASK_STACK_SIZE,
        NULL,
        CONFIG_PC_COMM_TASK_PRIORITY,
        NULL);

    if (ret != pdPASS)
    {
        vQueueDelete(app_queue);
        app_queue = NULL;
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}
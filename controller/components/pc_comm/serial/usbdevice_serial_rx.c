#include "usbdevice_serial_rx.h"

#include <string.h>

#include "tinyusb_cdc_acm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "../pc_comm_protocol.h"

static const char *TAG = "USBDEVICE_SERIAL_RX";

#define CONFIG_PC_COMM_QUEUE_SIZE          5
#define CONFIG_PC_COMM_RX_CHUNK_SIZE       CONFIG_TINYUSB_CDC_RX_BUFSIZE
#define CONFIG_PC_COMM_MESSAGE_MAX_SIZE    1024
#define CONFIG_PC_COMM_TASK_STACK_SIZE     4096
#define CONFIG_PC_COMM_TASK_PRIORITY       1

static uint8_t rx_buf[CONFIG_PC_COMM_RX_CHUNK_SIZE];
static uint8_t rx_message[CONFIG_PC_COMM_MESSAGE_MAX_SIZE];

static size_t rx_line_length;
static bool rx_discarding;

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
    (void)event;
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

    for (size_t i = 0; i < rx_size; ++i)
{
    const uint8_t c = rx_buf[i];

    if (c == '\n')
    {
        if (rx_discarding)
        {
            rx_discarding = false;
            rx_line_length = 0;
            continue;
        }

        if (rx_line_length == 0)
            continue;

        app_message_t msg = {
            .buf_len = rx_line_length,
            .itf = itf,
        };

        memcpy(
            msg.buf,
            rx_message,
            rx_line_length);

        if (xQueueSend(app_queue, &msg, 0) != pdTRUE)
        {
            ESP_LOGW(
                TAG,
                "Application queue full, dropping message");
        }

        rx_line_length = 0;
        continue;
    }

    if (rx_discarding)
        continue;

    if (c == '\r')
        continue;

    if (rx_line_length >= CONFIG_PC_COMM_MESSAGE_MAX_SIZE)
    {
        ESP_LOGW(
            TAG,
            "RX message too long, discarding");

        rx_line_length = 0;
        rx_discarding = true;
        continue;
    }

    rx_message[rx_line_length++] = c;
}
}

esp_err_t usbdevice_serial_rx_init(void)
{
    app_queue = xQueueCreate(
        CONFIG_PC_COMM_QUEUE_SIZE,
        sizeof(app_message_t));

    if (app_queue == NULL){return ESP_ERR_NO_MEM;}

    rx_line_length = 0;
    rx_discarding = false;
    
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
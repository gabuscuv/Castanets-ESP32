#include "usbdevice_serial_from.h"
#include "tinyusb_cdc_acm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include <stdlib.h>

static const char *TAG = "USBDEVICE_SERIAL_FROM";


static uint8_t rx_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];

/**
 * @brief Application Queue
 */
static QueueHandle_t app_queue;
typedef struct {
    uint8_t buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];     // Data buffer
    size_t buf_len;                                     // Number of bytes received
    uint8_t itf;                                        // Index of CDC device interface
} app_message_t;

void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event)
{
    /* initialization */
    size_t rx_size = 0;

    /* read */
    esp_err_t ret = tinyusb_cdcacm_read(itf, rx_buf, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);
    if (ret == ESP_OK) {

        app_message_t tx_msg = {
            .buf_len = rx_size,
            .itf = itf,
        };

        memcpy(tx_msg.buf, rx_buf, rx_size);
        xQueueSend(app_queue, &tx_msg, 0);
    } else {
        ESP_LOGE(TAG, "Read Error");
    }
}


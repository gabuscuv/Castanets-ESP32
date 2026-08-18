#include "usbdevice_serial_to.h"
#include "esp_err.h"
#include "tinyusb_cdc_acm.h"
#include "cJSON.h"
#include "inputframe.h"

static const char *TAG = "USBDEVICE_SERIAL_TO";

enum SERIALMESSAGETYPE {
    SERIALMESSAGETYPE_LOG,
    SERIALMESSAGETYPE_SYNCSTATUS,
    SERIALMESSAGETYPE_CONTROLLER
};

union serial_message {
  enum SERIALMESSAGETYPE messagetype;
  struct {
    const char * message;
  } a;
};

const char *jsonGenerator(InputFrame inputFrane) {
    cJSON *root = cJSON_CreateObject();
    return cJSON_Print(root);
}

esp_err_t serial_send(InputFrame inputFrane)
{
    const char* message = jsonGenerator(inputFrane);

    size_t messageSize = strlen(message);
    size_t offset = 0;
    
    while (offset < messageSize)
    {
        size_t remaining = messageSize - offset;

        if (remaining > CONFIG_TINYUSB_CDC_TX_BUFSIZE)
            remaining = CONFIG_TINYUSB_CDC_TX_BUFSIZE;

        size_t written = tinyusb_cdcacm_write_queue(
            TINYUSB_CDC_ACM_0,
            (const uint8_t *)(message + offset),
            remaining
        );

        if (written == 0)
            break;

        offset += written;
    }

    return ESP_OK;
};


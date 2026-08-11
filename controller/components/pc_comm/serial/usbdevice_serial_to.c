#include "usbdevice_serial_to.h"
#include "tinyusb_cdc_acm.h"
#include "cJSON.h"
#include "shared/struct.h"

static const char *TAG = "USBDEVICE_SERIAL_TO";

static bool running = false;

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

void serial_send_loop()
{
    while (running)
    {
        const char* message = jsonGenerator();

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
    }
};

const char *jsonGenerator() {
    cJSON *root = cJSON_CreateObject();
    return cJSON_Print(root);
}
#include "serial_init.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "esp_log.h"
#include "usbdevice_serial_commstates.h"
#include "usbdevice_serial_from.h"
#include "usbdevice_serial_to.h"

static const char *TAG = "USBDEVICE_INIT";

static TaskHandle_t serialcomm_task_handle = NULL;
static bool running = false;


int usbdevice_init()
{
    ESP_LOGI(TAG, "USB initialization");
    const tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = {
        .cdc_port = TINYUSB_CDC_ACM_0,
        .callback_rx = &tinyusb_cdc_rx_callback,
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = &tinyusb_cdc_line_state_changed_callback,
        .callback_line_coding_changed = NULL
    };

    ESP_ERROR_CHECK(tinyusb_cdcacm_init(&acm_cfg));

    BaseType_t result = xTaskCreate(serial_send_loop, "serialcomm_task", 4096,
                                    NULL, 5, &serialcomm_task_handle);

    running = true;
    return result;
};


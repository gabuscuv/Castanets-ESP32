#include "serial_init.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "esp_log.h"
#include "usbdevice_serial_commstates.h"

static const char *TAG = "USBDEVICE_INIT";

static bool running = false;


esp_err_t usbdevice_init(tusb_cdcacm_callback_t callback_rx)
{
    ESP_LOGI(TAG, "USB initialization");

    if (running){return ESP_OK;}

    const tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();

    esp_err_t err = tinyusb_driver_install(&tusb_cfg);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to install TinyUSB driver: %s",
            esp_err_to_name(err));

        return err;
    }

    tinyusb_config_cdcacm_t acm_cfg = {
        .cdc_port = TINYUSB_CDC_ACM_0,
        .callback_rx = callback_rx,
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed =
            &usbdevice_serial_commstates_changed,
        .callback_line_coding_changed = NULL
    };

    err = tinyusb_cdcacm_init(&acm_cfg);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to initialize CDC ACM: %s",
            esp_err_to_name(err));

        tinyusb_driver_uninstall();

        return err;
    }

    running = true;

    return ESP_OK;
}

esp_err_t usbdevice_deinit(void)
{
    ESP_LOGI(TAG, "USB deinitialization");

    if (!running)
        return ESP_OK;

    esp_err_t err = tinyusb_cdcacm_deinit(
        TINYUSB_CDC_ACM_0);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to deinitialize CDC ACM: %s",
            esp_err_to_name(err));

        return err;
    }

    err = tinyusb_driver_uninstall();

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to uninstall TinyUSB driver: %s",
            esp_err_to_name(err));

        return err;
    }

    running = false;

    return ESP_OK;
}
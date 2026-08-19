#include "runtime_server.h"

#include "esp_err.h"
#include "esp_log.h"

#include "runtime_inputframectx.h"
#include "runtime_pccomm.h"
#include "runtime_satellite.h"

static const char *TAG = "runtime_server";

esp_err_t runtime_server_init(void)
{
    esp_err_t err;

    ESP_LOGI(TAG, "Initializing runtime server");

    ESP_LOGI( TAG, "Initializing InputFrame context");

    err = runtime_inputframectx_init();

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to initialize InputFrame context: %s",
            esp_err_to_name(err));

        return err;
    }

    ESP_LOGI(TAG,"Initializing PC communication");

    err = runtime_pccomm_init();

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to initialize PC communication: %s",
            esp_err_to_name(err));

        runtime_inputframectx_deinit();

        return err;
    }

    ESP_LOGI(TAG, "Initializing satellite runtime");

    err = runtime_satellite_init();

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to initialize satellite runtime: %s",
            esp_err_to_name(err));

        runtime_pccomm_deinit();
        runtime_inputframectx_deinit();

        return err;
    }

    ESP_LOGI(TAG, "Runtime server initialized successfully");

    return ESP_OK;
}

esp_err_t runtime_server_deinit(void)
{
    esp_err_t err;
    esp_err_t first_err = ESP_OK;

    ESP_LOGI(TAG, "Deinitializing runtime server");

    ESP_LOGI(TAG, "Deinitializing satellite runtime");

    err = runtime_satellite_deinit();

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to deinitialize satellite runtime: %s",
            esp_err_to_name(err));

        first_err = err;
    }

    ESP_LOGI(TAG, "Deinitializing PC communication");

    err = runtime_pccomm_deinit();

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to deinitialize PC communication: %s",
            esp_err_to_name(err));

        if (first_err == ESP_OK){
            first_err = err;
        }
    }

    ESP_LOGI(TAG, "Deinitializing InputFrame context");

    err = runtime_inputframectx_deinit();

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to deinitialize InputFrame context: %s",
            esp_err_to_name(err));

        if (first_err == ESP_OK)
            first_err = err;
    }

    if (first_err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Runtime server deinitialization completed with errors");

        return first_err;
    }

    ESP_LOGI(TAG, "Runtime server deinitialized successfully");

    return ESP_OK;
}
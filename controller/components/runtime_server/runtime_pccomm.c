#include "runtime_pccomm.h"

#include <stdbool.h>

#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "pc_comm.h"
#include "runtime_inputframectx.h"

static const char *TAG = "runtime_pccomm";

static TaskHandle_t s_pc_report_task_handle = NULL;
static bool s_initialized = false;

static void runtime_pccomm_report_task(void *arg)
{
    (void)arg;

    while (true)
    {
        InputFrame* frame = runtime_inputframectx_get();

        esp_err_t err = pc_comm_send(*frame);

        if (err != ESP_OK)
        {
            ESP_LOGW(
                TAG,
                "Failed to send input frame to PC: %s",
                esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

esp_err_t runtime_pccomm_init(void)
{
    if (s_initialized)
    {
        return ESP_OK;
    }

    esp_err_t err = pc_comm_init();

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to initialize PC communication: %s",
            esp_err_to_name(err));

        return err;
    }

    BaseType_t result = xTaskCreate(
        runtime_pccomm_report_task,
        "pc_report",
        4096,
        NULL,
        5,
        &s_pc_report_task_handle);

    if (result != pdPASS)
    {
        ESP_LOGE(
            TAG,
            "Failed to create PC report task");

        pc_comm_deinit();

        s_pc_report_task_handle = NULL;

        return ESP_ERR_NO_MEM;
    }

    s_initialized = true;

    ESP_LOGI(
        TAG,
        "Runtime PC communication initialized");

    return ESP_OK;
}

esp_err_t runtime_pccomm_deinit(void)
{
    if (!s_initialized)
    {
        return ESP_OK;
    }

    if (s_pc_report_task_handle != NULL)
    {
        TaskHandle_t task = s_pc_report_task_handle;

        s_pc_report_task_handle = NULL;

        vTaskDelete(task);
    }

    esp_err_t err = pc_comm_deinit();

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to deinitialize PC communication: %s",
            esp_err_to_name(err));

        return err;
    }

    s_initialized = false;

    ESP_LOGI(
        TAG,
        "Runtime PC communication deinitialized");

    return ESP_OK;
}
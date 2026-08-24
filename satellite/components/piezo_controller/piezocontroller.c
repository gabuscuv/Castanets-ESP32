#include "piezocontroller.h"

#include "GPIO_CONFIG.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_adc/adc_oneshot.h"


#define PIEZO_TASK_STACK    2048
#define PIEZO_TASK_PRIORITY 10

static const char *TAG = "PIEZOCONTROLLER";

static piezocontroller_click_callback_t s_callback = NULL;
static TaskHandle_t s_task = NULL;
#ifdef PIEZO_AD0_GPIO
static adc_oneshot_unit_handle_t s_adc_handle = NULL;
#endif
static void IRAM_ATTR piezo_isr_handler(void *arg)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    if (s_task != NULL)
    {
        vTaskNotifyGiveFromISR(s_task, &higher_priority_task_woken);
    }

    if (higher_priority_task_woken)
    {
        portYIELD_FROM_ISR();
    }
        
}

static void piezo_task(void *arg)
{
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

#ifdef PIEZO_AD0_GPIO
        int raw = 0;
        esp_err_t err =
            adc_oneshot_read(
                s_adc_handle,
                PIEZO_ADC_CHANNEL,
                &raw
            );

        if (err != ESP_OK)
        {
            ESP_LOGE(
                TAG,
                "Failed to read piezo ADC: %s",
                esp_err_to_name(err)
            );
            continue;
        }
        ESP_LOGD(TAG, "Piezo detected, ADC=%d", raw);
#endif

        if (s_callback != NULL)
        {
            s_callback();
        }

        /*
         * Ignore additional oscillations from the
         * same physical impact.
         */
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

esp_err_t piezocontroller_init(piezocontroller_click_callback_t callback) {

#ifdef PIEZO_UNSUPPORTED 
    (void)callback;
    return ESP_ERR_NOT_SUPPORTED;
#else
    
    if (callback == NULL){return ESP_ERR_INVALID_ARG;}

    s_callback = callback;

    /*
     * D0: LM393 digital comparator output.
     *
     * Most LM393 modules pull D0 LOW when the signal
     * crosses the configured threshold, so use a
     * falling-edge interrupt.
     */
    gpio_config_t config = {
        .pin_bit_mask = 1ULL << PIEZO_D0_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };

    esp_err_t err = gpio_config(&config);

    if (err != ESP_OK){return err;}

    /*
     * AD0: analog output from the piezo amplifier.
     */
    #ifdef PIEZO_AD0_GPIO
    adc_oneshot_unit_init_cfg_t adc_config = {
        .unit_id = PIEZO_ADC_UNIT,
    };

    err = adc_oneshot_new_unit(&adc_config, &s_adc_handle);

    if (err != ESP_OK){return err;}

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    err = adc_oneshot_config_channel(
        s_adc_handle,
        PIEZO_ADC_CHANNEL,
        &channel_config
    );

    if (err != ESP_OK){return err;}
    #endif // PIEZO_AD0_GPIO
    /*
     * Create the task before enabling the ISR.
     */
    BaseType_t task_created = xTaskCreate(
        piezo_task,
        "piezo_task",
        PIEZO_TASK_STACK,
        NULL,
        PIEZO_TASK_PRIORITY,
        &s_task
    );

    if (task_created != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create piezo task");
        return ESP_ERR_NO_MEM;
    }

    err = gpio_install_isr_service(0);

    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE){return err;}

    err = gpio_isr_handler_add(
        PIEZO_D0_GPIO,
        piezo_isr_handler,
        NULL
    );

    if (err != ESP_OK){return err;}

    ESP_LOGI(TAG, "Piezo controller initialized: D0=GPIO%d", PIEZO_D0_GPIO);
#ifdef PIEZO_AD0_GPIO
    ESP_LOGI(TAG,
             "Piezo controller initialized: AD0=GPIO%d",
             PIEZO_AD0_GPIO,
             PIEZO_D0_GPIO);
#endif // PIEZO_AD0_GPIO
    return ESP_OK;
#endif // PIEZO_UNSUPPORTED
}
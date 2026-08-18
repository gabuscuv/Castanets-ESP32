#include "runtime_server.h"

#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "pc_comm.h"
#include "satellite_server.h"
#include "inputframe.h"

static const char *TAG = "runtime_server";

static InputFrame current_ctx;

static inline void controller_push_click(ControllerState *s, HubTime t)
{
    s->click[s->click_head] = t;

    s->click_head = (s->click_head + 1) % CONTROLLER_CLICK_HISTORY;

    if (s->click_count < CONTROLLER_CLICK_HISTORY)
        s->click_count++;
}

esp_err_t satellite_push_callback(controller_role_t controller, uint32_t time)
{
    ControllerState *s =
        (controller == CONTROLLER_LEFT
            ? &current_ctx.leftController
            : &current_ctx.rightController);

    controller_push_click(s, time);

    return ESP_OK;
}

esp_err_t runtime_server_init()
{

    ESP_LOGI(TAG, "Initializing runtime server");
    esp_err_t err;
    memset(&current_ctx, 0, sizeof(current_ctx));

    ESP_LOGI(TAG, "Initializing PC COMM");
    
    /*
     * PC communication.
     */
    // err = pc_comm_init();
    // if (err != ESP_OK)
    //     return err;

    // if (err != ESP_OK){ return 1;}

      err = satellite_server_init(
        satellite_push_callback);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to initialize satellite server: %s",
            esp_err_to_name(err));

        return err;
    }

    return ESP_OK;
}
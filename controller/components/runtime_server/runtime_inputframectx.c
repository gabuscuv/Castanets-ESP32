#include "runtime_inputframectx.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "esp_err.h"

#include "inputframe.h"
#include "controller_role.h"
#include "esp_timer.h"

static InputFrame s_current_ctx;
static bool s_initialized = false;

static int64_t s_hub_time_start_us;

static uint32_t runtime_inputframectx_get_time_ms(void)
{
    const int64_t elapsed_us =
        esp_timer_get_time() - s_hub_time_start_us;

    if (elapsed_us <= 0)
        return 0;

    return (uint32_t)(elapsed_us / 1000);
}

esp_err_t runtime_inputframectx_set_time(uint32_t time_ms)
{
    if (!s_initialized){return ESP_ERR_INVALID_STATE;}

    s_hub_time_start_us = esp_timer_get_time() - ((int64_t)time_ms * 1000);

    s_current_ctx.hubTime = time_ms;

    return ESP_OK;
}

esp_err_t runtime_inputframectx_reset_time(void)
{
    return runtime_inputframectx_set_time(0);
}

esp_err_t runtime_inputframectx_init(void)
{
    if (s_initialized){return ESP_OK;}

    memset(&s_current_ctx, 0, sizeof(s_current_ctx));

    s_hub_time_start_us = esp_timer_get_time();

    s_initialized = true;

    return ESP_OK;
}

esp_err_t runtime_inputframectx_deinit(void)
{
    if (!s_initialized){return ESP_OK;}

    s_initialized = false;

    memset(&s_current_ctx, 0, sizeof(s_current_ctx));

    return ESP_OK;
}

InputFrame *runtime_inputframectx_get(void)
{
    if (!s_initialized) {return NULL;}

    s_current_ctx.hubTime = runtime_inputframectx_get_time_ms();

    return &s_current_ctx;
}

ControllerState *runtime_inputframectx_get_controller(controller_role_t controller)
{
    if (!s_initialized) {return NULL;}

    switch (controller)
    {
    case CONTROLLER_LEFT:
        return &s_current_ctx.leftController;

    case CONTROLLER_RIGHT:
        return &s_current_ctx.rightController;

    default:
        return NULL;
    }
}
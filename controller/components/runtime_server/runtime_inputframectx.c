#include "runtime_inputframectx.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "esp_err.h"

#include "inputframe.h"
#include "controller_role.h"

static InputFrame s_current_ctx;
static bool s_initialized = false;

esp_err_t runtime_inputframectx_init(void)
{
    if (s_initialized){return ESP_OK;}

    memset(&s_current_ctx,0,sizeof(s_current_ctx));

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

InputFrame* runtime_inputframectx_get(void)
{
    if (!s_initialized) {return NULL;}

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
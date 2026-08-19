#include "runtime_satellite.h"
#include "esp_err.h"
#include "runtime_inputframectx.h"
#include "satellite_server.h"
#include "controller_role.h"
#include "inputframe.h"

static inline void controller_push_click(ControllerState *s, HubTime t)
{
    s->click[s->click_head] = t;

    s->click_head = (s->click_head + 1) % CONTROLLER_CLICK_HISTORY;

    if (s->click_count < CONTROLLER_CLICK_HISTORY)
        s->click_count++;
}

esp_err_t runtime_satellite_init()
{
  return satellite_server_init(runtime_satellite_push_callback);
}

esp_err_t runtime_satellite_deinit()
{
  return satellite_server_deinit();
}

esp_err_t runtime_satellite_push_callback(controller_role_t controller, uint32_t time)
{
    ControllerState *s = runtime_inputframectx_get_controller();

    controller_push_click(s, time);

    return ESP_OK;
}

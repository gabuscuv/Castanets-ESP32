#include "pc_comm_protocol_parse.h"

#include "esp_err.h"
#include "inputframe.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "cJSON.h"
#include "protocol_message.h"

static cJSON *controller_state_to_json(const ControllerState *controller)
{
    cJSON *json = cJSON_CreateObject();
    if (json == NULL)
        return NULL;

    cJSON *clicks = cJSON_AddArrayToObject(json, "clicks");
    if (clicks == NULL)
    {
        cJSON_Delete(json);
        return NULL;
    }

    /*
     * click_head points to the next position to write.
     * Therefore the oldest valid click is click_head - click_count.
     */
    int first =
        (controller->click_head -
         controller->click_count +
         CONTROLLER_CLICK_HISTORY) %
        CONTROLLER_CLICK_HISTORY;

    for (int i = 0; i < controller->click_count; ++i)
    {
        int index =
            (first + i) % CONTROLLER_CLICK_HISTORY;

        cJSON *click = cJSON_CreateObject();
        if (click == NULL)
        {
            cJSON_Delete(json);
            return NULL;
        }

        cJSON_AddNumberToObject(
            click,
            "time",
            (double)controller->click[index]);

        cJSON_AddStringToObject(
            click,
            "type",
            "click");

        cJSON_AddItemToArray(clicks, click);
    }

    cJSON *imu = cJSON_AddObjectToObject(json, "imu");
    if (imu == NULL)
    {
        cJSON_Delete(json);
        return NULL;
    }

    cJSON_AddNumberToObject(
        imu,
        "time",
        (double)controller->imu.time);

    cJSON_AddNumberToObject(
        imu,
        "x",
        (double)controller->imu.x);

    cJSON_AddNumberToObject(
        imu,
        "y",
        (double)controller->imu.y);

    cJSON_AddNumberToObject(
        imu,
        "z",
        (double)controller->imu.z);

    return json;
}

cJSON *input_frame_to_json(const InputFrame *frame)
{
    if (frame == NULL)
        return NULL;

    cJSON *json = cJSON_CreateObject();
    if (json == NULL)
        return NULL;

    cJSON_AddNumberToObject(json, "version", 1);
    cJSON_AddStringToObject(json, "type", "inputFrame");

    cJSON_AddNumberToObject(
        json,
        "hubTime",
        (double)frame->hubTime);

    /*
     * Left controller
     */
    cJSON *left =
        controller_state_to_json(&frame->leftController);

    if (left == NULL)
    {
        cJSON_Delete(json);
        return NULL;
    }

    cJSON_AddItemToObject(json, "leftController", left);

    /*
     * Right controller
     */
    cJSON *right =
        controller_state_to_json(&frame->rightController);

    if (right == NULL)
    {
        cJSON_Delete(json);
        return NULL;
    }

    cJSON_AddItemToObject(json, "rightController", right);

    /*
     * Feet controllers
     */
    cJSON *feet =
        cJSON_AddArrayToObject(json, "feetController");

    if (feet == NULL)
    {
        cJSON_Delete(json);
        return NULL;
    }

    for (int i = 0; i < FEET_CONTROLLER_COUNT; ++i)
    {
        const FootSample *sample =
            &frame->feetController[i];

        cJSON *foot = cJSON_CreateObject();
        if (foot == NULL)
        {
            cJSON_Delete(json);
            return NULL;
        }

        cJSON_AddNumberToObject(
            foot,
            "time",
            (double)sample->time);

        cJSON_AddNumberToObject(
            foot,
            "value",
            (double)sample->value);

        cJSON_AddItemToArray(feet, foot);
    }

    return json;
}


esp_err_t pccomm_cmd_from_json(const cJSON *json, pc_message_t *cmd)
{
    if (json == NULL || cmd == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const cJSON *cmd_json = cJSON_GetObjectItemCaseSensitive(json, "cmd");

    if (!cJSON_IsString(cmd_json) || cmd_json->valuestring == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const char *value = cmd_json->valuestring;

    if (strcmp(value, "handshake") == 0)
    {
        cmd->msg_type = PCCOMM_CMD_HANDSHAKE;
    }
    else if (strcmp(value, "start_song") == 0)
    {
        cmd->msg_type = PCCOMM_CMD_START_SONG;
    }
    else if (strcmp(value, "set_game_time") == 0)
    {
      cmd->msg_type = PCCOMM_CMD_SET_GAME_TIME;
      cJSON* timeJSON = cJSON_GetObjectItemCaseSensitive(json, "time");
      cmd->gametime = timeJSON->valuedouble;
    }
    else if (strcmp(value, "reset_timehub") == 0)
    {
        cmd->msg_type = PCCOMM_CMD_RESET_TIMEHUB;
    }
    else if (strcmp(value, "request_status") == 0)
    {
        cmd->msg_type = PCCOMM_CMD_REQUEST_STATUS;
    }
    else
    {
        return ESP_ERR_NOT_FOUND;
    }

    return ESP_OK;
}
#include "pc_comm_protocol.h"
#include "cJSON.h"
#include "esp_err.h"
#include "esp_log.h"

#include "inputframe.h"
#include "pc_comm_protocol_parse.h"
#include "protocol_message.h"
#include "serial/usbdevice_serial_send.h"

pccomm_protocol_callback_t pccomm_callback;
static const char *TAG = "PCCOMM_PROTOCOL";

esp_err_t pccomm_protocol_init(pccomm_protocol_callback_t callback)
{
    if (callback == NULL){return ESP_ERR_INVALID_ARG;}
    pccomm_callback = callback;

    return ESP_OK;
}

esp_err_t pccomm_protocol_handle(
    int itf,
    const uint8_t *data,
    size_t data_len)
{
    if (data == NULL || data_len == 0){return ESP_ERR_INVALID_ARG;}

    if (pccomm_callback == NULL){return ESP_ERR_INVALID_STATE;}

    ESP_LOGI(
        TAG,
        "MESSAGE from USB itf=%d: %.*s",
        itf,
        (int)data_len,
        (const char *)data);

    cJSON *message = cJSON_ParseWithLength(
        (const char *)data,
        data_len);

    if (message == NULL)
    {
        ESP_LOGW(TAG, "Invalid JSON");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *version =
        cJSON_GetObjectItemCaseSensitive(message, "version");

    cJSON *type =
        cJSON_GetObjectItemCaseSensitive(message, "type");

    if (!cJSON_IsNumber(version) ||
        !cJSON_IsString(type) ||
        type->valuestring == NULL)
    {
        ESP_LOGW(TAG, "Invalid message header");
        cJSON_Delete(message);
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(
        TAG,
        "version=%d type=%s",
        version->valueint,
        type->valuestring);

    pc_message_t command;

    esp_err_t err =
        pccomm_cmd_from_json(message, &command);

    cJSON_Delete(message);

    if (err != ESP_OK){return err;}

    pccomm_callback(command);

    return ESP_OK;
}

esp_err_t pccomm_protocol_sendFrame(InputFrame* inputframe)
{
    if (inputframe == NULL){return ESP_ERR_INVALID_ARG;}

    cJSON* json = input_frame_to_json(inputframe);

    if (json == NULL){return ESP_ERR_NO_MEM;}

    char* message = cJSON_PrintUnformatted(json);

    cJSON_Delete(json);

    if (message == NULL){return ESP_ERR_NO_MEM;}

    esp_err_t ret = serial_send(message);

    free(message);

    return ret;
}
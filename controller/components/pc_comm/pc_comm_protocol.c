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
    if (data == NULL || data_len == 0)
        return ESP_ERR_INVALID_ARG;

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
        !cJSON_IsString(type))
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

    /*
     * Dispatch message here.
     */
    pc_message_t a;
    esp_err_t err = pccomm_cmd_from_json(message, &a);
    
    cJSON_Delete(message);
    if (err != ESP_OK) {return ESP_ERR_INVALID_RESPONSE;}
    
    pccomm_callback(a);
    return ESP_OK;
}

esp_err_t pccomm_protocol_sendFrame(InputFrame* inputframe)
{
    return serial_send(cJSON_PrintUnformatted(input_frame_to_json(inputframe)));;
}
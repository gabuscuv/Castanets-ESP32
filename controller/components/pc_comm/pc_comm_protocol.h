#include "esp_err.h"
#include "app_message_t.h"
#include "inputframe.h"
#include "protocol_message.h"

typedef esp_err_t (*pccomm_protocol_rx_callback_t)(
    int itf,
    const uint8_t *data,
    size_t data_len);
typedef esp_err_t (*pccomm_protocol_callback_t)(pc_message_t);

esp_err_t pccomm_protocol_deinit(void);
esp_err_t pccomm_protocol_init(pccomm_protocol_callback_t callback);

esp_err_t pccomm_protocol_handle(
    int itf,
    const uint8_t *data,
    size_t data_len);

esp_err_t pccomm_protocol_sendFrame(InputFrame* inputframe);
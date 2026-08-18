#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "cJSON.h"
#include "satellite_espnow_protocol.h"

/**
 * Parse message type from JSON
 */
satellite_message_type_t
satellite_protocol_parse_type(const cJSON *type);

/**
 * Parse uint64 value from JSON number
 */
bool
satellite_protocol_parse_uint64(
    const cJSON *item,
    uint64_t *value);

/**
 * Parse click message from JSON
 */
bool
satellite_protocol_parse_click(
    const cJSON *root,
    satellite_message_t *message);

/**
 * Parse IMU message from JSON
 */
bool
satellite_protocol_parse_imu(
    const cJSON *root,
    satellite_message_t *message);
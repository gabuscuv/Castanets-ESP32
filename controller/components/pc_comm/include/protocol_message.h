#pragma once

typedef enum {
  /* PC -> MCU */
  PCCOMM_CMD_HANDSHAKE = 0,
  PCCOMM_CMD_START_SONG,
  PCCOMM_CMD_SET_GAME_TIME,
  PCCOMM_CMD_RESET_TIMEHUB,
  PCCOMM_CMD_REQUEST_STATUS,

  /* MCU -> PC */
  PCCOMM_EVT_HANDSHAKE_ACK,
  PCCOMM_EVT_STATUS,

} pccomm_msg_type_t;

typedef struct
{
    pccomm_msg_type_t msg_type;
    union
    {
        long long gametime;
    };
} pc_message_t;
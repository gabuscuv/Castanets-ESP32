#pragma once
// The length of ESPNOW primary master must be 16 bytes.
#define CONFIG_ESPNOW_PMK "pmk1234567890123"
// The length of ESPNOW local master must be 16 bytes.
#define CONFIG_ESPNOW_LMK "lmk1234567890123"
// 0 - 14
#define CONFIG_ESPNOW_CHANNEL 2

#define CONFIG_ESPNOW_ENABLE_LONG_RANGE 0
#define CONFIG_ESPNOW_ENABLE_POWER_SAVE 0

#if CONFIG_ESPNOW_ENABLE_POWER_SAVE
#define CONFIG_ESPNOW_WAKE_WINDOW 50
#define CONFIG_ESPNOW_WAKE_INTERVAL 100
#endif
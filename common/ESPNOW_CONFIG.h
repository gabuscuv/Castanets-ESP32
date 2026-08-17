#pragma once

/*
 * ESP-NOW WiFi channel.
 */
#define CONFIG_ESPNOW_CHANNEL 2

/*
 * Discovery:
 *
 * Client broadcasts a DISCOVER packet at this interval
 * until a server assigns it a role.
 */
#define CONFIG_ESPNOW_DISCOVERY_INTERVAL_MS 500

/*
 * If the client doesn't hear anything from the server
 * for this long, it returns to discovery mode.
 */
#define CONFIG_ESPNOW_CONNECTION_TIMEOUT_MS 3000

/*
 * Optional ESP-NOW Long Range mode.
 */
#define CONFIG_ESPNOW_ENABLE_LONG_RANGE 0

/*
 * Optional ESP-NOW power save.
 */
#define CONFIG_ESPNOW_ENABLE_POWER_SAVE 0

/*
 * RX queue.
 */
#define CONFIG_ESPNOW_QUEUE_SIZE 6

#if CONFIG_ESPNOW_ENABLE_POWER_SAVE
    #define CONFIG_ESPNOW_WAKE_WINDOW 50
    #define CONFIG_ESPNOW_WAKE_INTERVAL 100
#endif
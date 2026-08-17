#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/**
 * Initialize the onboard user LED.
 *
 * On XIAO ESP32-C6:
 *   GPIO15, active-low.
 *
 * On XIAO ESP32-C3:
 *   No software-controllable onboard user LED.
 *
 * @return ESP_OK on success.
 */
esp_err_t ledcontroller_init(void);

/**
 * Turn the LED on.
 */
void ledcontroller_on(void);

/**
 * Turn the LED off.
 */
void ledcontroller_off(void);

/**
 * Set LED state.
 *
 * @param on true to turn on, false to turn off.
 */
void ledcontroller_set(bool on);

/**
 * Toggle the LED.
 */
void ledcontroller_toggle(void);

/**
 * Get the current logical LED state.
 *
 * @return true if the controller considers the LED on.
 */
bool ledcontroller_is_on(void);

/**
 * Start a non-blocking blink pattern.
 *
 * @param on_ms  Time LED stays on.
 * @param off_ms Time LED stays off.
 */
void ledcontroller_blink(uint32_t on_ms, uint32_t off_ms);

/**
 * Stop any active blink pattern and turn the LED off.
 */
void ledcontroller_stop(void);

/**
 * Update the LED blink state.
 *
 * This should be called periodically by the application/task.
 */
void ledcontroller_update(void);
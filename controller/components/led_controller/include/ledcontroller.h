#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} ledcontroller_color_t;


/**
 * Initialize the onboard RGB LED.
 */
esp_err_t ledcontroller_init(void);


/**
 * Turn the LED off.
 */
void ledcontroller_off(void);

/**
 * Set the LED to an RGB color.
 *
 * @param r Red   component [0, 255]
 * @param g Green component [0, 255]
 * @param b Blue  component [0, 255]
 */
void ledcontroller_set_rgb(uint8_t r, uint8_t g, uint8_t b);

/**
 * Set the LED to a color.
 */
void ledcontroller_set_color(ledcontroller_color_t color);

/**
 * Get the currently configured color.
 */
ledcontroller_color_t ledcontroller_get_color(void);

/**
 * Set LED brightness.
 *
 * Brightness is applied to subsequent color updates.
 *
 * @param brightness [0, 255]
 */
void ledcontroller_set_brightness(uint8_t brightness);


/**
 * Get current brightness.
 */
uint8_t ledcontroller_get_brightness(void);

/**
 * Start a non-blocking blink effect.
 *
 * @param color Color used while ON.
 * @param on_ms ON duration.
 * @param off_ms OFF duration.
 */
void ledcontroller_blink(
    ledcontroller_color_t color,
    uint32_t on_ms,
    uint32_t off_ms
);

/**
 * Stop the current effect and turn the LED off.
 */
void ledcontroller_stop(void);


/**
 * Update the LED controller.
 *
 * Must be called periodically when using effects.
 */
void ledcontroller_update(void);


/**
 * Return whether the LED is logically on.
 */
bool ledcontroller_is_on(void);
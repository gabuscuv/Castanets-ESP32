#include "ledcontroller.h"

#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "led_strip.h"
#include "led_strip_rmt.h"

#define LEDCONTROLLER_TAG "ledcontroller"

/*
 * ESP32-S3-DevKitC-1:
 *
 * v1.0 -> GPIO48
 * v1.1 -> GPIO38
 *
 * For the current N8R8 reference board use GPIO38.
 */
#ifndef LEDCONTROLLER_GPIO
#define LEDCONTROLLER_GPIO GPIO_NUM_38
#endif

#define LEDCONTROLLER_LED_COUNT 1

static led_strip_handle_t s_strip = NULL;

static bool s_initialized = false;
static bool s_on = false;

static ledcontroller_color_t s_color = {
    .r = 0,
    .g = 0,
    .b = 0,
};

static uint8_t s_brightness = 255;

/* Blink state */

static bool s_blinking = false;
static bool s_blink_on = false;

static ledcontroller_color_t s_blink_color = {
    .r = 0,
    .g = 0,
    .b = 0,
};

static uint32_t s_blink_on_ms = 0;
static uint32_t s_blink_off_ms = 0;

static int64_t s_next_transition_us = 0;


/* -------------------------------------------------------------------------- */
/* Internal helpers                                                           */
/* -------------------------------------------------------------------------- */

static uint8_t ledcontroller_scale(
    uint8_t value,
    uint8_t brightness)
{
    return (uint8_t)(
        ((uint16_t)value * brightness) / 255
    );
}


static esp_err_t ledcontroller_write(
    ledcontroller_color_t color)
{
    if (!s_initialized || s_strip == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t r = ledcontroller_scale(
        color.r,
        s_brightness
    );

    uint8_t g = ledcontroller_scale(
        color.g,
        s_brightness
    );

    uint8_t b = ledcontroller_scale(
        color.b,
        s_brightness
    );

    /*
     * led_strip_set_pixel() handles the addressable LED's
     * color ordering for the configured strip.
     */
    esp_err_t err = led_strip_set_pixel(
        s_strip,
        0,
        r,
        g,
        b
    );

    if (err != ESP_OK) {
        return err;
    }

    return led_strip_refresh(s_strip);
}


/* -------------------------------------------------------------------------- */
/* Initialization                                                             */
/* -------------------------------------------------------------------------- */

esp_err_t ledcontroller_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    led_strip_config_t strip_config = {
        .strip_gpio_num = LEDCONTROLLER_GPIO,
        .max_leds = LEDCONTROLLER_LED_COUNT,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false,
        },
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .flags = {
            .with_dma = false,
        },
    };

    esp_err_t err = led_strip_new_rmt_device(
        &strip_config,
        &rmt_config,
        &s_strip
    );

    if (err != ESP_OK) {
        ESP_LOGE(
            LEDCONTROLLER_TAG,
            "Failed to initialize RGB LED: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    s_initialized = true;

    /*
     * Start with LED OFF.
     */
    err = led_strip_clear(s_strip);

    if (err != ESP_OK) {
        return err;
    }

    return ESP_OK;
}


/* -------------------------------------------------------------------------- */
/* Basic control                                                              */
/* -------------------------------------------------------------------------- */

void ledcontroller_off(void)
{
    if (!s_initialized) {
        return;
    }

    s_blinking = false;
    s_on = false;

    led_strip_clear(s_strip);
}


void ledcontroller_set_rgb(
    uint8_t r,
    uint8_t g,
    uint8_t b)
{
    if (!s_initialized) {
        return;
    }

    s_blinking = false;

    s_color.r = r;
    s_color.g = g;
    s_color.b = b;

    s_on = (r != 0 || g != 0 || b != 0);

    ledcontroller_write(s_color);
}


void ledcontroller_set_color(
    ledcontroller_color_t color)
{
    ledcontroller_set_rgb(
        color.r,
        color.g,
        color.b
    );
}


ledcontroller_color_t ledcontroller_get_color(void)
{
    return s_color;
}


/* -------------------------------------------------------------------------- */
/* Brightness                                                                 */
/* -------------------------------------------------------------------------- */

void ledcontroller_set_brightness(
    uint8_t brightness)
{
    s_brightness = brightness;

    if (!s_initialized) {
        return;
    }

    if (s_on) {
        ledcontroller_write(s_color);
    }
}


uint8_t ledcontroller_get_brightness(void)
{
    return s_brightness;
}


/* -------------------------------------------------------------------------- */
/* Blink                                                                      */
/* -------------------------------------------------------------------------- */

void ledcontroller_blink(
    ledcontroller_color_t color,
    uint32_t on_ms,
    uint32_t off_ms)
{
    if (!s_initialized) {
        return;
    }

    if (on_ms == 0) {
        on_ms = 1;
    }

    if (off_ms == 0) {
        off_ms = 1;
    }

    s_blink_color = color;

    s_blink_on_ms = on_ms;
    s_blink_off_ms = off_ms;

    s_blinking = true;
    s_blink_on = true;

    s_on = true;

    ledcontroller_write(s_blink_color);

    s_next_transition_us =
        esp_timer_get_time() +
        ((int64_t)s_blink_on_ms * 1000);
}


/* -------------------------------------------------------------------------- */
/* Stop                                                                       */
/* -------------------------------------------------------------------------- */

void ledcontroller_stop(void)
{
    ledcontroller_off();
}


/* -------------------------------------------------------------------------- */
/* Update                                                                     */
/* -------------------------------------------------------------------------- */

void ledcontroller_update(void)
{
    if (!s_initialized || !s_blinking) {
        return;
    }

    const int64_t now = esp_timer_get_time();

    if (now < s_next_transition_us) {
        return;
    }

    s_blink_on = !s_blink_on;

    if (s_blink_on) {

        s_on = true;

        ledcontroller_write(
            s_blink_color
        );

        s_next_transition_us =
            now +
            ((int64_t)s_blink_on_ms * 1000);

    } else {

        s_on = false;

        led_strip_clear(s_strip);

        s_next_transition_us =
            now +
            ((int64_t)s_blink_off_ms * 1000);
    }
}


/* -------------------------------------------------------------------------- */
/* State                                                                      */
/* -------------------------------------------------------------------------- */

bool ledcontroller_is_on(void)
{
    return s_on;
}
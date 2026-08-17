#include "ledcontroller.h"

#include "sdkconfig.h"

#include "driver/gpio.h"
#include "esp_timer.h"

#if CONFIG_IDF_TARGET_ESP32C6

#define LEDCONTROLLER_GPIO       GPIO_NUM_15
#define LEDCONTROLLER_ACTIVE_LOW true

#elif CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3

/*
 * the Waveshare ESP32-S3-Touch-LCD-1.85 and
 * the XIAO ESP32-C3 has no software-controllable onboard user LED.
 */

#else

#error "ledcontroller: unsupported ESP32 target"

#endif


static bool s_initialized = false;
static bool s_state = false;

static bool s_blinking = false;
static bool s_blink_state = false;

static uint32_t s_on_ms = 0;
static uint32_t s_off_ms = 0;

static int64_t s_next_transition_us = 0;


/* -------------------------------------------------------------------------- */
/* Internal                                                                   */
/* -------------------------------------------------------------------------- */

#if CONFIG_IDF_TARGET_ESP32C6

static void ledcontroller_write(bool on)
{
    /*
     * The XIAO ESP32-C6 LED is active-low:
     *
     *   GPIO LOW  -> LED ON
     *   GPIO HIGH -> LED OFF
     */
    gpio_set_level(
        LEDCONTROLLER_GPIO,
        on
            ? (LEDCONTROLLER_ACTIVE_LOW ? 0 : 1)
            : (LEDCONTROLLER_ACTIVE_LOW ? 1 : 0)
    );

    s_state = on;
}

#endif


/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

esp_err_t ledcontroller_init(void)
{
#if CONFIG_IDF_TARGET_ESP32C6

    gpio_config_t config = {
        .pin_bit_mask = 1ULL << LEDCONTROLLER_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&config);

    if (err != ESP_OK) {
        return err;
    }

    s_initialized = true;

    /*
     * Start in the OFF state.
     */
    ledcontroller_off();

    return ESP_OK;

#elif CONFIG_IDF_TARGET_ESP32C3

    /*
     * The XIAO ESP32-C3 has no software-controllable
     * onboard user LED.
     *
     * Treat the controller as successfully initialized
     * so application code does not need #ifdefs.
     */
    s_initialized = true;
    s_state = false;

    return ESP_OK;

#endif
}


void ledcontroller_on(void)
{
    if (!s_initialized) {
        return;
    }

    s_blinking = false;

#if CONFIG_IDF_TARGET_ESP32C6

    ledcontroller_write(true);

#else

    s_state = false;

#endif
}


void ledcontroller_off(void)
{
    if (!s_initialized) {
        return;
    }

    s_blinking = false;

#if CONFIG_IDF_TARGET_ESP32C6

    ledcontroller_write(false);

#else

    s_state = false;

#endif
}


void ledcontroller_set(bool on)
{
    if (on) {
        ledcontroller_on();
    } else {
        ledcontroller_off();
    }
}


void ledcontroller_toggle(void)
{
    if (!s_initialized) {
        return;
    }

    ledcontroller_set(!s_state);
}


bool ledcontroller_is_on(void)
{
    return s_state;
}


void ledcontroller_blink(uint32_t on_ms, uint32_t off_ms)
{
    if (!s_initialized) {
        return;
    }

    /*
     * Avoid a zero-duration state that would cause
     * the update loop to continuously transition.
     */
    if (on_ms == 0) {
        on_ms = 1;
    }

    if (off_ms == 0) {
        off_ms = 1;
    }

#if CONFIG_IDF_TARGET_ESP32C6

    s_on_ms = on_ms;
    s_off_ms = off_ms;

    s_blinking = true;
    s_blink_state = true;

    ledcontroller_write(true);

    s_next_transition_us =
        esp_timer_get_time() +
        ((int64_t)s_on_ms * 1000);

#else

    /*
     * Nothing to blink on the C3.
     */
    (void)on_ms;
    (void)off_ms;

    s_blinking = false;
    s_state = false;

#endif
}


void ledcontroller_stop(void)
{
    if (!s_initialized) {
        return;
    }

    s_blinking = false;

    ledcontroller_off();
}


void ledcontroller_update(void)
{
    if (!s_initialized || !s_blinking) {
        return;
    }

#if CONFIG_IDF_TARGET_ESP32C6

    const int64_t now = esp_timer_get_time();

    if (now < s_next_transition_us) {
        return;
    }

    /*
     * Switch between ON and OFF.
     */
    s_blink_state = !s_blink_state;

    ledcontroller_write(s_blink_state);

    const uint32_t duration_ms =
        s_blink_state
            ? s_on_ms
            : s_off_ms;

    s_next_transition_us =
        now +
        ((int64_t)duration_ms * 1000);

#endif
}
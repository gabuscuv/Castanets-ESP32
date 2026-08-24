#pragma once
#include "driver/gpio.h"
#if defined(CONFIG_IDF_TARGET_ESP32C6)

/*  Seeed XIAO ESP32-C6 Miniboard Pinout
    | Pin |   GPIO | Analog | Digital | Other functions        |
    | --- | ------ | ------ | ------- | ---------------------- |
    | D9  | GPIO20 | —      | D9      | MISO / SPI, SDIO_DATA0 |
    | D1  |  GPIO1 | A1     | D1      | —                      |
*/
#define PIEZO_D0_GPIO GPIO_NUM_20
/*
    | D10 | GPIO18 | —      | D10     | MOSI / SPI, SDIO_CMD   |
    | D0  |  GPIO0 | A0     | D0      | —                      |
*/
// #define PIEZO_AD0_GPIO      GPIO_NUM_18

#define PIEZO_ADC_UNIT      ADC_UNIT_1
#define PIEZO_ADC_CHANNEL   ADC_CHANNEL_2

#elif defined(CONFIG_IDF_TARGET_ESP32C3)

/* Seeed XIAO ESP32-C3 Miniboard Pinout
    | Pin |   GPIO | Analog | Digital | Other functions |     |
    | --- | ------ | ------ | ------- | --------------- | --- |
    | D9  |  GPIO9 | —      | D9      | MISO / SPI      |     |
    | D1  |  GPIO3 | A1     | D1      | —               |     |
*/
#define PIEZO_D0_GPIO GPIO_NUM_9
/*
    | D10 | GPIO10 | —      | D10     | MOSI / SPI      |     |
    | D0  |  GPIO2 | A0     | D0      | —               |
*/
// #define PIEZO_AD0_GPIO      GPIO_NUM_10

#define PIEZO_ADC_UNIT      ADC_UNIT_1
#define PIEZO_ADC_CHANNEL   ADC_CHANNEL_2
#else
#define PIEZO_UNSUPPORTED 1
#endif

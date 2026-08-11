#pragma once

#include <stdint.h>

#define CONTROLLER_CLICK_HISTORY 3
#define FEET_CONTROLLER_COUNT     3

typedef uint64_t HubTime;

typedef struct {
    HubTime time;

    float x;
    float y;
    float z;
} ImuSample;

typedef struct {
    HubTime time;
    float value;
} FootSample;

typedef struct {
    HubTime click[CONTROLLER_CLICK_HISTORY];
    ImuSample imu;
} ControllerState;

typedef struct {
    HubTime hubTime;

    ControllerState leftController;
    ControllerState rightController;

    FootSample feetController[FEET_CONTROLLER_COUNT];
} InputFrame;
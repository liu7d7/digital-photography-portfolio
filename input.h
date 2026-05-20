#pragma once

#include "state.h"

typedef struct input_t {
  float front, right, up, turn_up, turn_left;
} input_t;

extern input_t input;

bool key_down_callback(
    int type,
    EmscriptenKeyboardEvent const *event,
    void *user_data);

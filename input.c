#include "input.h"
#include "images.h"
#include <lib_demo.h>
#include <miniprintf.h>
#include <string.h>

input_t input;

bool key_down_callback(
    int type,
    EmscriptenKeyboardEvent const *event,
    void *user_data)
{
  if (type != EMSCRIPTEN_EVENT_KEYDOWN) return false;

  state_t *s = user_data;

  if (s->anim_progress > 0.99) {
    if (strcmp(event->code, "ArrowLeft") == 0) {
      s->anim_progress = 0.f;
      s->prev_index = s->current_index;
      s->current_index = (s->current_index - 1 + n_texs) % n_texs;
    } else if (strcmp(event->code, "ArrowRight") == 0) {
      s->anim_progress = 0.f;
      s->prev_index = s->current_index;
      s->current_index = (s->current_index + 1) % n_texs;
    }

    emscripten_mini_stdio_printf("%d, %d\n", s->prev_index, s->current_index);
  }

  if (strcmp(event->code, "KeyW") == 0) input.front++;
  if (strcmp(event->code, "KeyA") == 0) input.right--;
  if (strcmp(event->code, "KeyS") == 0) input.front--;
  if (strcmp(event->code, "KeyD") == 0) input.right++;
  if (strcmp(event->code, "ShiftLeft") == 0) input.up--;
  if (strcmp(event->code, "Space") == 0) input.up++;
  if (strcmp(event->code, "KeyQ") == 0) input.turn_left++;
  if (strcmp(event->code, "KeyE") == 0) input.turn_left--;
  if (strcmp(event->code, "KeyR") == 0) input.turn_up++;
  if (strcmp(event->code, "KeyF") == 0) input.turn_up--;

  emscripten_mini_stdio_printf("key down: code=%s\n", event->code);
  return false;
}

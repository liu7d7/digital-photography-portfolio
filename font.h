#pragma once

#include <lib_webgpu.h>

typedef struct pch_t
{
  unsigned short x0, y0, x1, y1;
  float xoff, yoff, xadvance, xoff2, yoff2;
} pch_t;

typedef struct metrics_t
{
  int ascent[2];
  float ascent_in_pixels[2];
} metrics_t;

typedef struct font_metadata_t
{
  WGpuTexture tex;
  metrics_t mt;
  pch_t pch[512];
} font_metadata_t;

void downloaded_font_image(
    WGpuImageBitmap bmp,
    int width,
    int height,
    void *user_data);

void downloaded_font_metadata(uint8_t *data, int size, void *user_data);

void font_new_rp(struct state_t *s);

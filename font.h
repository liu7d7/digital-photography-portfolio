#pragma once

#include <lib_webgpu.h>
#include <stdbool.h>

typedef struct pch_t
{
  unsigned short x0, y0, x1, y1;
  float xoff, yoff, xadvance, xoff2, yoff2;
} pch_t;

typedef struct metrics_t
{
  int line_gap[2];
  int ascent[2];
  float scale_to_one[2];
} metrics_t;

typedef struct font_metadata_t
{
  WGpuTexture tex;
  WGpuTextureView tex_view;
  metrics_t mt;
  pch_t pch[512];
  bool tex_ready, metrics_ready;
} font_metadata_t;

typedef struct downloaded_font_args_t 
{
  struct state_t *s;
  font_metadata_t *dst;
} downloaded_font_args_t;

void downloaded_font_image(
    WGpuImageBitmap bmp,
    int width,
    int height,
    void *user_data);

void downloaded_font_metadata(uint8_t *data, int size, void *user_data);

#pragma once

#include <lib_webgpu.h>
#include "vecmath.h"

#define n_texs 14

typedef struct aabb_2d_t 
{
  v2_t min;
  v2_t max;
} aabb_2d_t;

typedef struct pic_vertex_t
{
  v3_t pos;
  v2_t uv; // or -1
} pic_vertex_t;

extern pic_vertex_t cpu_vb[n_texs][6];
extern aabb_2d_t img_bounds[n_texs];

typedef struct pic_pose_t
{
  v3_t pos;
  float yaw, pitch;
} pic_pose_t;

static char const *tex_paths[] = {
  "0.webp",
  "1.webp",
  "2.webp",
  "3.webp",
  "4.webp",
  "5.webp",
  "6.webp",
  "7.webp",
  "8.2.webp",
  "9.webp",
  "10.webp",
  "11.webp",
  "12.webp",
  "13.webp",
};

typedef struct downloaded_image_args_t
{
  int index;
  struct state_t *state;
  struct xorshift32_state_t *rs;
} downloaded_image_args_t;

void downloaded_image(
    WGpuImageBitmap bitmap,
    int width,
    int height,
    void *_args);

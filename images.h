#pragma once

#include <lib_webgpu.h>
#include "vecmath.h"

#define n_texs 12

// number of vertices per frame: 24 quads (frame) + 12 quads (weird cubes) + 1 quad (pic)
// 37 quads -> 37*6 = 222 vertices
typedef struct pic_vertex_t
{
  v3_t pos;
  v2_t uv; // or -1
} pic_vertex_t;

extern pic_vertex_t cpu_vb[n_texs][6];

typedef struct pic_pose_t
{
  v3_t pos;
  float yaw, pitch;
} pic_pose_t;

static char const *tex_paths[] = {
  "0.jpg",
  "1.jpg",
  "2.jpg",
  "3.jpg",
  "4.jpg",
  "5.jpg",
  "6.jpg",
  "7.jpg",
  "8.jpg",
  "9.jpg",
  "10.jpg",
  "11.jpg",
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

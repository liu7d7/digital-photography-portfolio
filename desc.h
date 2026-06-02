#pragma once

#include "lib_webgpu.h"
#include "vecmath.h"

typedef struct desc_set_t
{
  WGpuBuffer vb;
  int n_descs;
  int *desc_byte_bounds /* [n_descs + 1] */;
  WGpuBindGroup *bgs /* [n_descs] */;
} desc_set_t;

typedef struct font_draw_cmd_t
{
  // assume color is always white.
  v3_t pos;
  float scale;
  char const *text; // can contain \b for bold and \n for newline
  int len;
  int desc_id;
} font_draw_cmd_t; 

desc_set_t desc_set_new(
    struct state_t *s,
    int n,
    struct font_metadata_t *fm,
    WGpuBuffer *model_mat_ubs /* [n] */, 
    font_draw_cmd_t *cmds /* [n] */);


#pragma once

#include <stdatomic.h>
#include <lib_webgpu.h>
#include "vecmath.h"
#include "images.h"
#include "font.h"
#include "desc.h"

#define x_state_all_wgpu_persistent_fields \
  x(WGpuAdapter, adapter) \
  x(WGpuDevice, dev) \
  x(WGpuRenderPipeline, main_rp) \
  x(WGpuRenderPipeline, stars_rp) \
  x(WGpuRenderPipeline, crt_rp) \
  x(WGpuCanvasContext, ctx) \
  x(WGpuBuffer, cam_ub) \
  x(WGpuBuffer, post_ub) \
  x(WGpuBindGroup, stars_bg) \
  x(WGpuBindGroup, crt_bg_0) \
  x(WGpuBuffer, post_vb) \
  x(WGpuSampler, d_samp)

#define x_state_all_wgpu_per_frame_fields \
  x(WGpuCommandEncoder, cmds) \

#define x_state_all_wgpu_dependent_fields \
  x(WGpuBindGroup, crt_bg_1) \
  x(WGpuTexture, scratch_tex_0) \
  x(WGpuTextureView, scratch_tex_0_view) \

#define x_state_all_wgpu_persistent_arrays \
  x(WGpuBuffer, vbs, n_texs) \
  x(WGpuTexture, texs, n_texs) \
  x(WGpuBuffer, model_mat_ubs, n_texs) \
  x(WGpuBindGroup, per_obj_bgs, n_texs) \
  x(WGpuBuffer, opacity_ubs, 2) \
  x(WGpuBuffer, font_opacity_ubs, 2) \
  x(WGpuBindGroup, per_frame_bgs, 2)

typedef struct state_t
{
#define x(a, b) a b;
  x_state_all_wgpu_persistent_fields;
  x_state_all_wgpu_dependent_fields;
  x_state_all_wgpu_per_frame_fields;
#undef x

#define x(a, b, c) a b[c];
  x_state_all_wgpu_persistent_arrays;
#undef x

  pic_pose_t poses[n_texs];

  cam_t cam;
  float time, time_loaded_imgs;
  float anim_progress;
  int prev_index, current_index;

  atomic_int n_texs_loaded;

  v2_t configured_window_size;

  font_metadata_t font;
  desc_set_t descs;
} state_t;

WGpuRenderPipeline state_new_render_pipeline(
    state_t *s,
    int n_buffers,
    WGpuVertexBufferLayout *vb_layout,
    char const *vss,
    char const *vs_entrypoint,
    char const *fss,
    char const *fs_entrypoint,
    int n_targets,
    WGpuColorTargetState *color_target_states);


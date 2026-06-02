#include "state.h"

WGpuRenderPipeline state_new_render_pipeline(
    state_t *s,
    int n_buffers,
    WGpuVertexBufferLayout *vb_layout,
    char const *vss,
    char const *vs_entrypoint,
    char const *fss,
    char const *fs_entrypoint,
    int n_targets,
    WGpuColorTargetState *color_target_states)
{
  WGpuColorTargetState default_color_target_state = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;

  default_color_target_state.format = navigator_gpu_get_preferred_canvas_format();
  default_color_target_state.blend.color.operation = WGPU_BLEND_OPERATION_ADD;
  default_color_target_state.blend.color.srcFactor = WGPU_BLEND_FACTOR_SRC_ALPHA;
  default_color_target_state.blend.color.dstFactor = WGPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  default_color_target_state.blend.alpha.operation = WGPU_BLEND_OPERATION_ADD;
  default_color_target_state.blend.alpha.srcFactor = WGPU_BLEND_FACTOR_ZERO;
  default_color_target_state.blend.alpha.dstFactor = WGPU_BLEND_FACTOR_ONE;

  WGpuShaderModuleDescriptor shader_desc_temp = {};

  if (!color_target_states) {
    color_target_states = &default_color_target_state;
    n_targets = 1;
  }

  WGpuRenderPipelineDescriptor rp_desc = 
    WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;

  rp_desc.vertex.numBuffers = n_buffers;
  rp_desc.vertex.buffers = vb_layout;

  shader_desc_temp.code = vss;
  rp_desc.vertex.module = 
    wgpu_device_create_shader_module(s->dev, &shader_desc_temp);
  rp_desc.vertex.entryPoint = "main";

  shader_desc_temp.code = fss;
  rp_desc.fragment.module = 
    wgpu_device_create_shader_module(s->dev, &shader_desc_temp);
  rp_desc.fragment.entryPoint = "main";

  rp_desc.fragment.numTargets = n_targets;
  rp_desc.fragment.targets = color_target_states;

  return wgpu_device_create_render_pipeline(s->dev, &rp_desc);
}

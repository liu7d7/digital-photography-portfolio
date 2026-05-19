#include <math.h>
#include <string.h>
#include <complex.h>
#include <stdatomic.h>
#include <emscripten/em_math.h>
#include <lib_webgpu.h>
#include <lib_demo.h>
#include <miniprintf.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#include "vecmath.h"
#include "images.h"

WGpuAdapter adapter;
WGpuDevice device;
WGpuCanvasContext canvas_context;
WGpuRenderPipeline main_render_pipeline, stars_render_pipeline;
WGpuBuffer vertex_buffers[n_textures];
WGpuTexture textures[n_textures], scratch_texture_1, scratch_texture_2;
WGpuSampler samplers[n_textures];
WGpuBuffer model_matrix_buffers[n_textures], window_size_buffer, camera_buffer, stars_uniform_buffer;
WGpuBindGroup per_frame_bind_group, per_object_bind_groups[n_textures];
WGpuBindGroup stars_bind_group;
WGpuCommandEncoder command_encoder;

#define em(x) emscripten_math_##x

WGPU_BOOL
draw(double time, void *user_data)
{
  // done?: create render pass and render
  // done?: create logic for sorting picture frames
  // TODO: create animation logic
  // TODO: create logic for drawing frame hanger things

  command_encoder = wgpu_device_create_command_encoder(device, NULL);

  WGpuRenderPassColorAttachment color_attachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  color_attachment.view = wgpu_canvas_context_get_current_texture_view(canvas_context);
  color_attachment.loadOp = WGPU_LOAD_OP_CLEAR;
  color_attachment.clearValue.r =
  color_attachment.clearValue.g =
  color_attachment.clearValue.b = 0.0;
  color_attachment.clearValue.a = 1.0;

  WGpuRenderPassDescriptor render_pass_descriptor = {};
  render_pass_descriptor.numColorAttachments = 1;
  render_pass_descriptor.colorAttachments = &color_attachment;

  {
    static double last_time = 0;
    if (last_time != 0) {
      float delta_time = (float)(time - last_time) * 1e-3f;
      v3_t flat_front = camera.front;
      flat_front.y = 0;
      v3_normalize(&flat_front);

      v3_t delta = v3_add(v3_add(v3_mul(camera.right, input.right), (v3_t){0, input.up, 0}), v3_mul(flat_front, input.front));
      v3_normalize(&delta);

      float yaw_delta = input.turn_left;
      float pitch_delta = input.turn_up;

      camera.pos = v3_add(camera.pos, v3_mul(delta, delta_time));
      camera.yaw += yaw_delta * delta_time;
      camera.pitch += pitch_delta * delta_time;

      if (camera.pitch > M_PI_2) camera.pitch = M_PI_2 - 0.000001;
      if (camera.pitch < -M_PI_2) camera.pitch = -M_PI_2 + 0.000001;

      camera_recalculate_matrix(&camera);

      if (input.up + input.front + input.right + input.turn_left + input.turn_up != 0) emscripten_mini_stdio_printf("%f, %f, %f, %f, %f\n", camera.pos.x, camera.pos.y, camera.pos.z, camera.yaw, camera.pitch);
      input.up = input.front = input.right = input.turn_left = input.turn_up = 0;
    }

    last_time = time;
  }

  // "struct stars_input_t {\n"
  // "  one_texel : vec2f,\n"
  // "  resolution : vec2f,\n"
  // "  time : f32,\n"
  // "};\n"

  struct {
    v2_t one_texel;
    v2_t resolution;
    float time;
  } stars_input;

  wgpu_queue_write_buffer(wgpu_device_get_queue(device), stars_uniform_buffer, 0, &stars_input, sizeof(stars_input));

  WGpuRenderPassEncoder render_pass_encoder = wgpu_command_encoder_begin_render_pass(command_encoder, &render_pass_descriptor);
  wgpu_render_pass_encoder_set_pipeline(render_pass_encoder, stars_render_pipeline);
  wgpu_render_pass_encoder_set_bind_group(render_pass_encoder, 0, stars_input, 0, 0);
  wgpu_render_pass_encoder_set_vertex_buffer(render_pass_encoder, 0, post_process_vertex_buffer, 0, sizeof(v2_t) * 6);
  wgpu_render_pass_encoder_draw(render_pass_encoder, 6, 1, 0, 0);
  wgpu_render_pass_encoder_end(render_pass_encoder);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(command_encoder));

//   if (atomic_load(&n_textures_loaded) == n_textures) {
//     float now = (float)emscripten_get_now();
//     float animation_bias = 0;
//     float x = (now - animation_start_time) / animation_time;
//     if (x <= 1 && requested_animation) {
//       animation_bias = animation_get(x) * requested_animation;
//     } else if (requested_animation) {
//       current_index -= requested_animation;
//       requested_animation = 0;
//     }
//
// #define j_for_i(i) ((i + current_index - 2) % n_textures + n_textures) % n_textures
//
//     for (int i = 0; i < n_textures; i++) {
//       int j = j_for_i(i);
//       float ia = (float)i + animation_bias - floor(n_textures * .5);
//       m4_t transform = {
//         1, 0, 0, 0,
//         0, 1, 0, 0,
//         0, 0, 1, 0,
//         0, 0, 0, 1
//       };
//
//       m4_translate(&transform, (v3_t){ia * 0.6, -ia * 0.4, ia});
//
//       wgpu_queue_write_buffer(wgpu_device_get_queue(device), model_matrix_buffers[j], 0, &transform, sizeof(transform));
//     }
//
//     wgpu_queue_write_buffer(wgpu_device_get_queue(device), camera_buffer, 0, &camera, sizeof(m4_t) * 2);
//
//     WGpuRenderPassEncoder render_pass_encoder = 
//       wgpu_command_encoder_begin_render_pass(command_encoder, &render_pass_descriptor);
//
//     wgpu_render_pass_encoder_set_pipeline(render_pass_encoder, main_render_pipeline);
//     wgpu_render_pass_encoder_set_bind_group(render_pass_encoder, 0, per_frame_bind_group, 0, 0);
//
//     for (int i = 0; i < n_textures; i++) {
//       int j = j_for_i(i);
//       wgpu_render_pass_encoder_set_bind_group(render_pass_encoder, 1, per_object_bind_groups[j], 0, 0);
//       wgpu_render_pass_encoder_set_vertex_buffer(render_pass_encoder, 0, vertex_buffers[j], 0, sizeof(cpu_vertex_buffer[0]));
//       wgpu_render_pass_encoder_draw(render_pass_encoder, sizeof(cpu_vertex_buffer[0])/sizeof(cpu_vertex_buffer[0][0]), 1, 0, 0);
//     }
//
//     wgpu_render_pass_encoder_end(render_pass_encoder);
//   } else {
//     wgpu_render_pass_encoder_end(wgpu_command_encoder_begin_render_pass(command_encoder, &render_pass_descriptor));
//   }
//
//   wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(command_encoder));

  return EM_TRUE;
}


void
resized_canvas()
{
  camera.proj = create_perspective_matrix(EM_MATH_PI / 2.25, 0.01, 100);
}

void
obtained_web_gpu_device(WGpuDevice result, void *user_data)
{
  device = result;

  canvas_context = wgpu_canvas_get_webgpu_context("canvas");

  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = navigator_gpu_get_preferred_canvas_format();
  config.alphaMode = WGPU_CANVAS_ALPHA_MODE_PREMULTIPLIED;
  wgpu_canvas_context_configure(canvas_context, &config);

  char const *vertex_shader_source = 
    "struct in_t {\n"
    "  @location(0) pos : vec3<f32>,\n"
    "  @location(1) uv : vec2<f32>\n"
    "};\n"

    "struct out_t {\n"
    "  @builtin(position) pos : vec4<f32>,\n"
    "  @location(0) uv : vec2<f32>,\n"
    "  @location(1) z : f32,\n"
    "};\n"

    "struct camera_t {\n"
    "  view : mat4x4f,\n"
    "  proj : mat4x4f\n"
    "};\n"

    "@group(0) @binding(0) var<uniform> camera : camera_t;\n"
    "@group(1) @binding(2) var<uniform> model : mat4x4f;\n"

    "@vertex\n"
    "fn main(in : in_t) -> out_t {\n"
    "  var out : out_t;\n"
    // "  let pos = model * vec4<f32>(in.pos, 1.0);\n"
    // "  out.pos = camera.proj * camera.view * pos;\n"
    "  let pos = vec4<f32>(in.pos, 1.0) * model;\n"
    "  out.pos = pos * camera.view * camera.proj;\n"
    "  out.uv = in.uv;\n"
    "  out.z = pos.z;\n"
    "  return out;\n"
    "}\n";

  char const *fragment_shader_source =
    "@group(1) @binding(0) var b_texture : texture_2d<f32>;\n"
    "@group(1) @binding(1) var b_sampler : sampler;\n"

    "@fragment\n"
    "fn main(@location(0) uv : vec2<f32>, @location(1) z : f32) -> @location(0) vec4<f32> {\n"
    // "  return vec4<f32>(1.);\n"
    "  var out_color : vec4<f32> = select(textureSample(b_texture, b_sampler, vec2<f32>(uv.x, 1.-uv.y)), vec4<f32>(0., 0., 0., 1.), uv.x < 0 && uv.y < 0);\n"
    "  out_color.a = max(0., (1. - select(abs(z / 5.), 3*z, z > 0)));\n"  
    // "  out_color *= 0;\n"  
    "  return out_color;\n"
    "}\n";

  WGpuVertexAttribute vertex_attributes[2] = {};

  vertex_attributes[0].format = WGPU_VERTEX_FORMAT_FLOAT32X3;
  vertex_attributes[0].offset = 0;
  vertex_attributes[0].shaderLocation = 0;

  vertex_attributes[1].format = WGPU_VERTEX_FORMAT_FLOAT32X2;
  vertex_attributes[1].offset = 12;
  vertex_attributes[1].shaderLocation = 1;

  WGpuVertexBufferLayout vertex_buffer_layout = {};
  vertex_buffer_layout.numAttributes = 2;
  vertex_buffer_layout.attributes = vertex_attributes;
  vertex_buffer_layout.arrayStride = 20;

  WGpuRenderPipelineDescriptor render_pipeline_desc = 
    WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;

  render_pipeline_desc.vertex.numBuffers = 1;
  render_pipeline_desc.vertex.buffers = &vertex_buffer_layout;

  WGpuShaderModuleDescriptor shader_module_descriptor_temp = {};

  shader_module_descriptor_temp.code = vertex_shader_source;
  render_pipeline_desc.vertex.module = 
    wgpu_device_create_shader_module(device, &shader_module_descriptor_temp);
  render_pipeline_desc.vertex.entryPoint = "main";

  shader_module_descriptor_temp.code = fragment_shader_source;
  render_pipeline_desc.fragment.module = 
    wgpu_device_create_shader_module(device, &shader_module_descriptor_temp);
  render_pipeline_desc.fragment.entryPoint = "main";

  WGpuColorTargetState color_target = 
    WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  color_target.format = config.format;
  color_target.blend.color.operation = WGPU_BLEND_OPERATION_ADD;
  color_target.blend.color.srcFactor = WGPU_BLEND_FACTOR_SRC_ALPHA;
  color_target.blend.color.dstFactor = WGPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  color_target.blend.alpha.operation = WGPU_BLEND_OPERATION_ADD;
  color_target.blend.alpha.srcFactor = WGPU_BLEND_FACTOR_ZERO;
  color_target.blend.alpha.dstFactor = WGPU_BLEND_FACTOR_ONE;
  render_pipeline_desc.fragment.numTargets = 1;
  render_pipeline_desc.fragment.targets = &color_target;

  main_render_pipeline = 
    wgpu_device_create_render_pipeline(device, &render_pipeline_desc);

  char const *post_process_vertex_shader_source = 
    "struct out_t {\n"
    "  @builtin(position) pos : vec4f,\n"
    "  @location(0) uv : vec2f,\n"
    "};\n"

    "@vertex\n"
    "fn main(@location(0) pos : vec2f) : out_t {\n"
    "  var out : out_t;\n"
    "  out.pos = vec4f(pos, 0., 1.);\n"
    "  out.uv = pos * 0.5 + 0.5;\n"
    "  return out;\n"
    "}\n";

  char const *stars_fragment_shader_source =
    "struct stars_input_t {\n"
    "  one_texel : vec2f,\n"
    "  resolution : vec2f,\n"
    "  time : f32,\n"
    "};\n"

    "@group(0) @binding(0) var<uniform> uni : stars_input_t;\n"

    "fn cosg(t : f32) : vec3f {\n"
    "  let a = vec3f(0.938, 0.328, 0.718);\n"
    "  let b = vec3f(0.659, 0.438, 0.328);\n"
    "  let c = vec3f(0.388, 0.388, 0.296);\n"
    "  let d = vec3f(2.538, 2.478, 0.168);\n"
    "  return clamp(a + b * cos(2. * 3.1415926 * (c * t + d)), 0., 1.);\n"
    "}\n"

    "fn hash43x(p : vec3f) -> vec4f {\n"
    "  var x = vec3u(vec3i(p));\n"
    "  x = 1103515245u * ((x.xyz >> 1u) ^ x.yzx);\n"
    "  let h = 1103515245u * ((x.x ^ x.z) ^ (x.y >> 3U));\n"
    "  vec4u rz = vec4u(h, h*16807u, h*48271u, h*69621u);\n"
    "  return vec4f((rz >> 1) & uvec4(0x7fffffffu))/float(0x7fffffff);\n"
    "}\n"

    "fn star(_rd : vec3f) -> vec3f {\n"
    "  var accum = vec3f(0.);\n"
    "  var rd = _rd;\n"
    "  let n = 25.;\n"

    "  for (var i = 0; i < 8; i++) {\n"
    "    rd = mat3x3f(0.21821789,0.43643578,0.87287156,"
    "                 -0.9701425,0.,0.24253563,"
    "                 0.10585122,-0.89973541,0.4234049) * rd;\n"
    "    rd = normalize(rd);\n"
    "    rd = rd.zxy;\n"

    "    let r = abs(rd);\n"
    "    let p = r / max(r.x, max(r.y, r.z));\n"
    "    let a = 1. - step(vec3f(0.9999999), p);\n"
    "    let s = p * a * sign(rd);\n"

    "    let ip = floor(s * n);\n"
    "    let fp = fract(s * n);\n"
    "    let h = hash43x(ip + vec3f(vec3i(i) * 400));\n"

    "    let g = (fp - .5) - (h.xyz * .6 - .3);\n"
    "    if a.x == 0. { g.x = g.z; }\n"
    "    else if a.y == 0. { g.y = g.z; }\n"
    "    g.z = 0.;\n"

    "    let w = h.z * 2. * 3.1415926 + 0.5 * (sin(0.5 + uni.time + 40. * h.z) + cos(0.2 * uni.time - 2. + 40. * h.z));\n"
    "    let _c = cos(w); let _s = sin(w);\n"
    "    g.xy = mat2x2f(_c, _s, -_s, _c);\n"

    "    let e = smoothstep(0.8, 1.2, 1. / min(max((200. + 50. * h.w) * abs(g.x * g.y), 0.01), 1.2));\n"

    "    let b = pow((1. - length(g)) * 1.1, 4.) * pow(h.y, 4.)\n"
    "            * (sin(32. * h.x + uni.time) * .5 + .5)\n"
    "            * pow(max(r.x, max(r.y, r.z)), 10.)\n"
    "            * e;\n"

    "    accum = accum + vec3f(cosg(pow(b, 1.4))) * b;\n"
    "  }\n"

    "  return accum;\n"
    "}\n"

    "@fragment\n"
    "fn main(@location(0) _uv : vec2f) : @location(0) vec4f {\n"
    "  let uv = _uv * 2. - 1.;\n"
    "  uv.x *= uni.res.x / uni.res.y;\n"
    "  let fovy = 3.1415926 / 2.25;\n"
    "  let rd = normalize(vec3f(uv, 1. / tan(fovy)));\n"
    "  return vec4f(star(rd), 1.);\n"
    "}\n";

  WGpuVertexAttribute stars_vertex_attributes[1] = {};

  vertex_attributes[0].format = WGPU_VERTEX_FORMAT_FLOAT32X2;
  vertex_attributes[0].offset = 0;
  vertex_attributes[0].shaderLocation = 0;

  WGpuVertexBufferLayout vertex_buffer_layout = {};
  vertex_buffer_layout.numAttributes = 1;
  vertex_buffer_layout.attributes = vertex_attributes;
  vertex_buffer_layout.arrayStride = 8;

  WGpuRenderPipelineDescriptor stars_render_pipeline_descriptor =
    WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;

  stars_render_pipeline_descriptor.vertex.numBuffers = 1;

  stars_render_pipeline_desc.vertex.numBuffers = 1;
  stars_render_pipeline_desc.vertex.buffers = &vertex_buffer_layout;

  shader_module_descriptor_temp.code = post_process_vertex_shader_source;
  stars_render_pipeline_desc.vertex.module = 
    wgpu_device_create_shader_module(device, &shader_module_descriptor_temp);
  stars_render_pipeline_desc.vertex.entryPoint = "main";

  shader_module_descriptor_temp.code = stars_fragment_shader_source;
  stars_render_pipeline_desc.fragment.module = 
    wgpu_device_create_shader_module(device, &shader_module_descriptor_temp);
  stars_render_pipeline_desc.fragment.entryPoint = "main";

  WGpuColorTargetState color_target = 
    WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  color_target.format = config.format;
  color_target.blend.color.operation = WGPU_BLEND_OPERATION_ADD;
  color_target.blend.color.srcFactor = WGPU_BLEND_FACTOR_SRC_ALPHA;
  color_target.blend.color.dstFactor = WGPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  color_target.blend.alpha.operation = WGPU_BLEND_OPERATION_ADD;
  color_target.blend.alpha.srcFactor = WGPU_BLEND_FACTOR_ZERO;
  color_target.blend.alpha.dstFactor = WGPU_BLEND_FACTOR_ONE;
  stars_render_pipeline_desc.fragment.numTargets = 1;
  stars_render_pipeline_desc.fragment.targets = &color_target;

  stars_render_pipeline = 
    wgpu_device_create_render_pipeline(device, &stars_render_pipeline_desc);

  // for (int i = 0; i < n_textures; i++) {
  //   downloaded_image_args_t *args = malloc(sizeof(downloaded_image_args_t));
  //   args->index = i;
  //   emscripten_mini_stdio_printf("attempting download image: src=%s\n", texture_paths[i]);
  //   wgpu_load_image_bitmap_from_url_async(
  //       texture_paths[i], 
  //       WGPU_TRUE, 
  //       downloaded_image, 
  //       args);
  // }

  camera.proj = create_perspective_matrix(EM_MATH_PI / 2.25, 0.01, 100);
  camera_recalculate_matrix(&camera);

  WGpuBufferDescriptor camera_buffer_desc = {};
  camera_buffer_desc.size = sizeof(camera_t);
  camera_buffer_desc.usage = 
    WGPU_BUFFER_USAGE_UNIFORM | WGPU_BUFFER_USAGE_COPY_DST;
  camera_buffer_desc.mappedAtCreation = WGPU_FALSE;

  camera_buffer = wgpu_device_create_buffer(device, &camera_buffer_desc);

  WGpuBindGroupEntry bind_group_entries[1] = {};
  bind_group_entries[0].binding = 0;
  bind_group_entries[0].resource = camera_buffer;

  per_frame_bind_group = 
    wgpu_device_create_bind_group(
        device, 
        wgpu_pipeline_get_bind_group_layout(main_render_pipeline, 0), 
        bind_group_entries, 
        1);

  window_resized_callback(resized_canvas);

  emscripten_mini_stdio_printf("requesting raf\n");
  wgpu_request_animation_frame_loop(draw, NULL);
}

void
obtained_web_gpu_adapter(WGpuAdapter result, void *user_data)
{
  adapter = result;

  WGpuDeviceDescriptor device_desc = {};
  wgpu_adapter_request_device_async(adapter, &device_desc, obtained_web_gpu_device, NULL);
}

int
main(int argc, char **argv)
{
  camera.pos = (v3_t){0, 0, 3};
  camera.yaw = -M_PI_2;
  emscripten_mini_stdio_printf("hello world!\n");
  WGpuRequestAdapterOptions options = {};
  navigator_gpu_request_adapter_async(&options, obtained_web_gpu_adapter, NULL);
  emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, true, key_down_callback);
  emscripten_mini_stdio_printf("bye world!\n");
}

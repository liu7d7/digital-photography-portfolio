#include <math.h>
#include <stdatomic.h>
#include <lib_webgpu.h>
#include <lib_demo.h>
#include <miniprintf.h>
#include <assert.h>

#include "input.h"
#include "lib_dlbin.h"
#include "vecmath.h"
#include "images.h"
#include "shader_sources.h"
#include "state.h"

typedef struct post_input_t {
  m4_t view;
  v2_t one_texel;
  v2_t resolution;
  float time;
  float opac;
  float pad[2];
} post_input_t;

void obtained_web_gpu_device(
    WGpuDevice result,
    void *user_data);

int draw(double time, void *user_data)
{
  // done?: create render pass and render
  // done?: create logic for sorting picture frames
  // done?: create animation logic
  // forgone: create logic for drawing frame hanger things

  state_t *s = user_data;

  if (!v2_eq(get_window_size(), s->configured_window_size)) {
#define x(a, b) wgpu_object_destroy(s->b); s->b = 0;
    x_state_all_wgpu_dependent_fields;
#undef x

    emscripten_mini_stdio_printf("resizing\n");
    obtained_web_gpu_device(s->dev, s);
    return EM_FALSE;
  }

  s->time = (float)time;
  s->anim_progress = v_lerp(s->anim_progress, 1.f, 0.02f);
  s->cmds = wgpu_device_create_command_encoder(s->dev, NULL);

  WGpuRenderPassColorAttachment backbuf =
    WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  backbuf.clearValue.r =
  backbuf.clearValue.g =
  backbuf.clearValue.b = 0.0;
  backbuf.clearValue.a = 1.0;

  if (atomic_load(&s->n_texs_loaded) != n_texs 
      || !(s->font.metrics_ready && s->font.tex_ready)) {
    backbuf.view =
      wgpu_canvas_context_get_current_texture_view(s->ctx);
    backbuf.loadOp = WGPU_LOAD_OP_CLEAR;

    WGpuRenderPassDescriptor rp_desc = {};
    rp_desc.numColorAttachments = 1;
    rp_desc.colorAttachments = &backbuf;

    wgpu_render_pass_encoder_end(wgpu_command_encoder_begin_render_pass(s->cmds, &rp_desc));

    s->time_loaded_imgs = s->time;
    goto end;
  }

  if (!s->descs.vb) {
    // desc_set_t desc_set_new(
    //     struct state_t *s,
    //     struct font_metadata_t *fm,
    //     int n,
    //     WGpuBuffer *model_mat_ubs /* [n] */, 
    //     font_draw_cmd_t *cmds /* [n] */);

    // typedef struct font_draw_cmd_t
    // {
    //   // assume color is always white.
    //   v3_t pos;
    //   float scale;
    //   char const *text; // can contain \b for bold and \n for newline
    //   int len;
    //   int desc_id;
    // } font_draw_cmd_t; 

    char const *titles[] = {
      "\\bink drop",
      "\\bibm ad",
      "\\b5\xa2 back",
      "\\bfaces no.1",
      "\\bfaces no.2",
      "\\bb/w portrait",
      "\\balbum cover",
      "\\bstill life",
      "\\bsurrealism",
      "\\bkqed no.1",
      "\\bkqed no.2",
      "\\bkqed no.3",
      "\\bkqed no.4",
      "\\bkqed no.5",
    };

    typedef struct desc_t {
      char const *text;
      int n_lines;
    } desc_t;

    desc_t descs[] = {
      {"\\rdrops of ink in water\\nmake beautiful patterns\\n\\bphoto 1. ", 3},
      {"\\ran ad for an ibm pc\\nconvertible computer\\n\\bphoto 2. ", 3},
      {"\\rstreet-art-esque edit\\nof nickelback\\n\\bphoto 3. ", 3},
      {"\\rcommunity voices.\\nizumi wei\\n\\bphoto 4. ", 3},
      {"\\rcommunity voices.\\nmichelle boire\\n\\bphoto 5. ", 3},
      {"\\rblack & white text\\nportrait of th""\xe9""a\\n\\bphoto 6. ", 3},
      {"\\rremixed album cover for\\nabelard's meta valley\\n\\bphoto 7. ", 3},
      {"\\rchina pot, lights,\\nartificial flowers\\n\\bphoto 8. ", 3},
      {"\\rmixture of stock images,\\nmy photos, and blender\\n\\bphoto 9. ", 3},
      {"\\ramerican creed photo essay.\\nskateboarder posing in BART station\\n\\bphoto 10.", 3},
      {"\\ramerican creed photo essay.\\nfriend slurping noodles\\n\\bphoto 11.", 3},
      {"\\ramerican creed photo essay.\\nwoman feeding birds\\n\\bphoto 12.", 3},
      {"\\ramerican creed photo essay.\\ntrain arriving at platform\\n\\bphoto 13.", 3},
      {"\\ramerican creed photo essay.\\ntrain leaving station\\n\\bphoto 14.", 3},
    };

    float line_height_0 = s->font.mt.ascent[0] * s->font.mt.scale_to_one[0] / 3;

    font_draw_cmd_t *draw_cmds = malloc(sizeof(font_draw_cmd_t) * n_texs * 2);
    for (int i = 0; i < n_texs; i++) {
      font_draw_cmd_t title = {
        .scale = 0.3,
        .desc_id = i,
        .justify = 1,
        .pos = {img_bounds[i].max.x, img_bounds[i].min.y - 0.03},
        .text = titles[i]
      };

      font_draw_cmd_t desc = {
        .scale = 0.2,
        .desc_id = i,
        .justify = 0,
        .pos = {
          img_bounds[i].min.x,
          img_bounds[i].max.y + line_height_0 * .25 * .2 * descs[i].n_lines + 0.025
        },
        .text = descs[i].text
      };

      draw_cmds[i * 2] = title;
      draw_cmds[i * 2 + 1] = desc;
    }

    s->descs = desc_set_new(
        s,
        &s->font,
        14,
        28,
        s->model_mat_ubs,
        draw_cmds);

    emscripten_mini_stdio_printf("initialized desc_set");
  }

  /*--- draw :> update post-process buffer ---*/
  {
    post_input_t post_input = {};
    post_input.resolution = get_window_size();
    post_input.one_texel = (v2_t){
      1.f / post_input.resolution.x,
      1.f / post_input.resolution.y
    };

    post_input.view = s->cam.view;
    post_input.time = time * 1e-3f;
    post_input.opac = fminf(1.f, em(sqrt, (s->time - s->time_loaded_imgs) / 500.f));

    wgpu_queue_write_buffer(
        wgpu_device_get_queue(s->dev),
        s->post_ub,
        0,
        &post_input,
        sizeof(post_input));
  }

  /*--- draw :> update camera, upload camera buffer ---*/
  {
    float yaw = r_lerp(
        s->poses[s->prev_index].yaw, 
        s->poses[s->current_index].yaw, 
        s->anim_progress);

    float pitch = r_lerp(
        s->poses[s->prev_index].pitch, 
        s->poses[s->current_index].pitch, 
        s->anim_progress);

    v3_t pos = v3_lerp(
        s->poses[s->prev_index].pos, 
        s->poses[s->current_index].pos, 
        s->anim_progress);

    s->cam.yaw = yaw;
    s->cam.pitch = pitch;
    s->cam.pos = pos;

    cam_recalculate_matrix(&s->cam);

    wgpu_queue_write_buffer(
        wgpu_device_get_queue(s->dev),
        s->cam_ub,
        0,
        &s->cam,
        sizeof(m4_t) * 2);
  }

  /*--- draw :> update opacity buffers ---*/
  {
    float op_0 = s->anim_progress;
    float op_1 = 1.f - op_0;

    wgpu_queue_write_buffer(
        wgpu_device_get_queue(s->dev), 
        s->opacity_ubs[0],
        0,
        &op_0,
        sizeof(op_0));

    wgpu_queue_write_buffer(
        wgpu_device_get_queue(s->dev), 
        s->opacity_ubs[1],
        0,
        &op_1,
        sizeof(op_1));
  }

  /*--- draw :> stars ---*/
  {
    backbuf.view = s->scratch_tex_0_view;
    backbuf.loadOp = WGPU_LOAD_OP_CLEAR;

    WGpuRenderPassDescriptor render_pass_desc = {};
    render_pass_desc.numColorAttachments = 1;
    render_pass_desc.colorAttachments = &backbuf;

    WGpuRenderPassEncoder enc =
      wgpu_command_encoder_begin_render_pass(s->cmds, &render_pass_desc);

    wgpu_render_pass_encoder_set_pipeline(enc, s->stars_rp);
    wgpu_render_pass_encoder_set_bind_group(enc, 0, s->stars_bg, 0, 0);
    wgpu_render_pass_encoder_set_vertex_buffer(
        enc, 
        0, 
        s->post_vb,
        0,
        sizeof(v2_t) * 6);

    wgpu_render_pass_encoder_draw(enc, 6, 1, 0, 0);

    wgpu_render_pass_encoder_set_pipeline(enc, s->main_rp);

    // prev
    wgpu_render_pass_encoder_set_bind_group(enc, 0, s->per_frame_bgs[1], 0, 0);
    wgpu_render_pass_encoder_set_bind_group(
        enc,
        1,
        s->per_obj_bgs[s->prev_index],
        0, 0);

    wgpu_render_pass_encoder_set_vertex_buffer(
        enc, 
        0, 
        s->vbs[s->prev_index],
        0,
        sizeof(cpu_vb[0]));

    wgpu_render_pass_encoder_draw(enc, 6, 1, 0, 0);

    // prev text
    wgpu_render_pass_encoder_set_bind_group(
        enc, 
        1,
        s->descs.bgs[s->prev_index],
        0, 0);

    int begin = s->descs.desc_byte_bounds[s->prev_index];
    int count = s->descs.desc_byte_bounds[s->prev_index + 1] - begin;
    wgpu_render_pass_encoder_set_vertex_buffer(
        enc,
        0,
        s->descs.vb,
        begin,
        WGPU_MAX_SIZE);

    wgpu_render_pass_encoder_draw(enc, count / 20, 1, 0, 0);

    // current
    wgpu_render_pass_encoder_set_bind_group(enc, 0, s->per_frame_bgs[0], 0, 0);
    wgpu_render_pass_encoder_set_bind_group(
        enc,
        1,
        s->per_obj_bgs[s->current_index],
        0, 0);

    wgpu_render_pass_encoder_set_vertex_buffer(
        enc, 
        0, 
        s->vbs[s->current_index],
        0,
        sizeof(cpu_vb[0]));

    wgpu_render_pass_encoder_draw(enc, 6, 1, 0, 0);

    // current text
    wgpu_render_pass_encoder_set_bind_group(
        enc, 
        1,
        s->descs.bgs[s->current_index],
        0, 0);

    begin = s->descs.desc_byte_bounds[s->current_index];
    count = s->descs.desc_byte_bounds[s->current_index + 1] - begin;
    wgpu_render_pass_encoder_set_vertex_buffer(
        enc,
        0,
        s->descs.vb,
        begin,
        WGPU_MAX_SIZE);

    wgpu_render_pass_encoder_draw(enc, count / 20, 1, 0, 0);

    wgpu_render_pass_encoder_end(enc);

    wgpu_object_destroy(enc);
  }

  /*--- draw :> crt ---*/
  {
    backbuf.view =
      wgpu_canvas_context_get_current_texture_view(s->ctx);
    backbuf.loadOp = WGPU_LOAD_OP_CLEAR;

    WGpuRenderPassDescriptor rp_desc = {};
    rp_desc.numColorAttachments = 1;
    rp_desc.colorAttachments = &backbuf;

    WGpuRenderPassEncoder enc =
      wgpu_command_encoder_begin_render_pass(s->cmds, &rp_desc);

    wgpu_render_pass_encoder_set_pipeline(enc, s->crt_rp);
    wgpu_render_pass_encoder_set_bind_group(enc, 0, s->crt_bg_0, 0, 0);
    wgpu_render_pass_encoder_set_bind_group(enc, 1, s->crt_bg_1, 0, 0);
    wgpu_render_pass_encoder_set_vertex_buffer(
        enc, 
        0, 
        s->post_vb,
        0,
        sizeof(v2_t) * 6);

    wgpu_render_pass_encoder_draw(enc, 6, 1, 0, 0);
    wgpu_render_pass_encoder_end(enc);

    wgpu_object_destroy(enc);
  }

end:
  wgpu_queue_submit_one_and_destroy(
      wgpu_device_get_queue(s->dev), 
      wgpu_command_encoder_finish(s->cmds));

  return EM_TRUE;
}

void obtained_web_gpu_device(
    WGpuDevice result,
    void *user_data)
{
  state_t *s = user_data;
  s->dev = result;
  s->configured_window_size = get_window_size();
  s->ctx = wgpu_canvas_get_webgpu_context("canvas");

  /*--- init :> s->ctx ---*/ 
  {
    WGpuCanvasConfiguration config =
      WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;

    config.device = s->dev;
    config.format = navigator_gpu_get_preferred_canvas_format();
    config.alphaMode = WGPU_CANVAS_ALPHA_MODE_PREMULTIPLIED;

    wgpu_canvas_context_configure(s->ctx, &config);
  }

    /*--- init :> s->scratch_tex_0 ---*/
  if (!s->scratch_tex_0) {
    WGpuTextureDescriptor tex_desc = 
      WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;

    tex_desc.format = navigator_gpu_get_preferred_canvas_format();
    v2_t ws = get_window_size();
    tex_desc.width = (int)ws.x;
    tex_desc.height = (int)ws.y;
    tex_desc.usage = 
      WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT
      | WGPU_TEXTURE_USAGE_TEXTURE_BINDING;

    s->scratch_tex_0 = wgpu_device_create_texture(s->dev, &tex_desc);
    s->scratch_tex_0_view = wgpu_texture_create_view(s->scratch_tex_0, 0);
  }

  /*--- init :> s->d_samp ---*/
  if (!s->d_samp) {
    WGpuSamplerDescriptor sd = WGPU_SAMPLER_DESCRIPTOR_DEFAULT_INITIALIZER;
    s->d_samp = wgpu_device_create_sampler(s->dev, &sd);
  }

  /*--- init :> s->main_rp ---*/
  if (!s->main_rp) {
    WGpuVertexAttribute attrs[2] = {
      [0] = {
        .format = WGPU_VERTEX_FORMAT_FLOAT32X3, 
        .offset = 0, 
        .shaderLocation = 0
      },
      [1] = {
        .format = WGPU_VERTEX_FORMAT_FLOAT32X2, 
        .offset = 12, 
        .shaderLocation = 1
      }
    };

    WGpuVertexBufferLayout vb_layout = {
      .numAttributes = 2,
      .attributes = attrs,
      .arrayStride = 20
    };

    s->main_rp = state_new_render_pipeline(
        s,
        1, &vb_layout,
        vertex_shader_source, "main",
        fragment_shader_source, "main",
        0, NULL);
  }
  
  /*--- init :> s->stars_rp, s->crt_rp ---*/
  if (!s->stars_rp || !s->crt_rp) {
    WGpuVertexAttribute attrs[] = {
      {
        .format = WGPU_VERTEX_FORMAT_FLOAT32X2,
        .offset = 0,
        .shaderLocation = 0
      }
    };

    WGpuVertexBufferLayout vb_layout = {
      .numAttributes = 1,
      .attributes = attrs,
      .arrayStride = 8
    };

    wgpu_object_destroy(s->stars_rp);
    s->stars_rp = state_new_render_pipeline(
        s,
        1, &vb_layout,
        post_process_vertex_shader_source, "main",
        stars_fragment_shader_source, "main",
        0, NULL);

    wgpu_object_destroy(s->crt_rp);
    s->crt_rp = state_new_render_pipeline(
        s,
        1, &vb_layout,
        post_process_vertex_shader_source, "main",
        crt_fragment_shader_source, "main",
        0, NULL);
  }

  /*--- init :> s->fonts */
  if (!s->font.tex) /* @hack(liu7d7): condition is weird. */ {
    downloaded_font_args_t *args0 = malloc(sizeof(*args0)),
                           *args1 = malloc(sizeof(*args1));

    args0->s = args1->s = s;
    args0->dst = args1->dst = &s->font;

    download_binary_file(
        "font.dat",
        downloaded_font_metadata,
        args0);

    wgpu_load_image_bitmap_from_url_async(
        "font.png",
        WGPU_FALSE,
        downloaded_font_image,
        args1);
  }

  /*--- init :> s->texs ---*/
  if (!s->texs[0]) {
    for (int i = 0; i < 2; i++) {
      downloaded_image_args_t *args = malloc(sizeof(downloaded_image_args_t));
      args->index = i;
      args->state = s;

      emscripten_mini_stdio_printf(
          "attempting download image: src=%s\n",
          tex_paths[i]);

      wgpu_load_image_bitmap_from_url_async(
          tex_paths[i], 
          WGPU_TRUE, 
          downloaded_image, 
          args);
    }
  }

  s->cam.proj = create_perspective_matrix(EM_MATH_PI / 4., 0.01, 100);
  cam_recalculate_matrix(&s->cam);

  /*--- init :> s->opacity_ubs ---*/
  if (!s->opacity_ubs[0]) {
    WGpuBufferDescriptor ub_desc = {};
    ub_desc.size = sizeof(float);
    ub_desc.usage = 
      WGPU_BUFFER_USAGE_STORAGE | WGPU_BUFFER_USAGE_COPY_DST;
    ub_desc.mappedAtCreation = WGPU_FALSE;
    
    s->opacity_ubs[0] = wgpu_device_create_buffer(s->dev, &ub_desc);
    s->opacity_ubs[1] = wgpu_device_create_buffer(s->dev, &ub_desc);
  }

  /*--- init :> s->cam_ub ---*/
  if (!s->cam_ub) {
    WGpuBufferDescriptor ub_desc = {};
    ub_desc.size = sizeof(cam_t);
    ub_desc.usage = WGPU_BUFFER_USAGE_STORAGE | WGPU_BUFFER_USAGE_COPY_DST;
    ub_desc.mappedAtCreation = WGPU_FALSE;

    s->cam_ub = wgpu_device_create_buffer(s->dev, &ub_desc);
  }

  /*--- init :> s->post_ub ---*/
  if (!s->post_ub) {
    WGpuBufferDescriptor ub_desc = {};
    ub_desc.size = sizeof(post_input_t);
    ub_desc.usage = WGPU_BUFFER_USAGE_STORAGE | WGPU_BUFFER_USAGE_COPY_DST;
    ub_desc.mappedAtCreation = WGPU_FALSE;

    s->post_ub = wgpu_device_create_buffer(s->dev, &ub_desc);
  }

  /*--- init :> s->post_vb ---*/
  if (!s->post_vb) {
    WGpuBufferDescriptor post_vb_desc = {};
    post_vb_desc.size = sizeof(v2_t) * 6;
    post_vb_desc.usage = 
      WGPU_BUFFER_USAGE_VERTEX;
    post_vb_desc.mappedAtCreation = WGPU_TRUE;

    s->post_vb = wgpu_device_create_buffer(s->dev, &post_vb_desc);
    wgpu_buffer_get_mapped_range(s->post_vb, 0, WGPU_MAX_SIZE);

    v2_t post_vb_src[] = {
      {-1, -1},
      {1, -1},
      {1, 1},
      {1, 1},
      {-1, 1},
      {-1, -1}
    };

    wgpu_buffer_write_mapped_range(s->post_vb, 0, 0, post_vb_src, sizeof(post_vb_src));
    wgpu_buffer_unmap(s->post_vb);
  }

  /*--- init :> s->per_frame_bg ---*/
  if (!s->per_frame_bgs[0]) {
    WGpuBindGroupEntry per_frame_bg_entries[] = {
      { .binding = 0, .resource = s->cam_ub },
      { .binding = 1, .resource = s->opacity_ubs[0] }
    };

    s->per_frame_bgs[0] = 
      wgpu_device_create_bind_group(
          s->dev, 
          wgpu_pipeline_get_bind_group_layout(s->main_rp, 0), 
          per_frame_bg_entries, 
          2);

    per_frame_bg_entries[1].resource = s->opacity_ubs[1];
    s->per_frame_bgs[1] = 
      wgpu_device_create_bind_group(
          s->dev, 
          wgpu_pipeline_get_bind_group_layout(s->main_rp, 0), 
          per_frame_bg_entries, 
          2);
  }

  /*--- init :> s->post_bg ---*/
  if (!s->stars_bg || !s->crt_bg_0) {
    WGpuBindGroupEntry bg_entries[] = {
      { .binding = 0, .resource = s->post_ub }
    };

    wgpu_object_destroy(s->stars_bg);
    s->stars_bg = 
      wgpu_device_create_bind_group(
          s->dev,
          // note(liu7d7) :> below stars_rp is just an example of post
          wgpu_pipeline_get_bind_group_layout(s->stars_rp, 0), 
          bg_entries,
          1);

    wgpu_object_destroy(s->crt_bg_0);
    s->crt_bg_0 = 
      wgpu_device_create_bind_group(
          s->dev,
          // note(liu7d7) :> below stars_rp is just an example of post
          wgpu_pipeline_get_bind_group_layout(s->crt_rp, 0), 
          bg_entries,
          1);
  }

  /*--- init :> s->crt_bg_1 ---*/
  if (!s->crt_bg_1) {
    WGpuBindGroupEntry bg_entries[] = {
      { 
        .binding = 0,
        .resource = wgpu_texture_create_view(s->scratch_tex_0, 0) 
      },
      { 
        .binding = 1,
        .resource = s->d_samp
      }
    };

    s->crt_bg_1 = wgpu_device_create_bind_group(
        s->dev,
        wgpu_pipeline_get_bind_group_layout(s->crt_rp, 1), 
        bg_entries,
        2);
  }

#define x(a, b) assert(s->b != 0);
  x_state_all_wgpu_persistent_fields;
  x_state_all_wgpu_dependent_fields;
#undef x

  emscripten_mini_stdio_printf("requesting raf\n");
  
  wgpu_request_animation_frame_loop(draw, s);
}

void obtained_web_gpu_adapter(WGpuAdapter result, void *user_data)
{
  state_t *s = user_data;

  s->adapter = result;
  WGpuAdapterInfo adapter_info = {};
  wgpu_adapter_get_info(s->adapter, &adapter_info);
  emscripten_mini_stdio_printf("%s; %s; %s\n", adapter_info.vendor, adapter_info.device, adapter_info.architecture);

  WGpuDeviceDescriptor dev_desc = {};
  wgpu_adapter_request_device_async(
      s->adapter, 
      &dev_desc,
      obtained_web_gpu_device,
      s);
}

state_t state;

int main(int argc, char **argv)
{
  state.anim_progress = 1.f;

  WGpuRequestAdapterOptions options = {
    .powerPreference = WGPU_POWER_PREFERENCE_HIGH_PERFORMANCE 
  };

  navigator_gpu_request_adapter_async(
      &options, 
      obtained_web_gpu_adapter,
      &state);

  emscripten_set_keydown_callback(
      EMSCRIPTEN_EVENT_TARGET_DOCUMENT,
      &state,
      true,
      key_down_callback);
}

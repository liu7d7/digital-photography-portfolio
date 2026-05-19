#pragma once

#include "vecmath.h"

static atomic_int n_textures_loaded = 0;
#define n_textures 12
static char const *texture_paths[] = {
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

typedef
struct downloaded_image_args_t
{
  int index;
} downloaded_image_args_t;

void
downloaded_image(WGpuImageBitmap bitmap, int width, int height, void *_args)
{
  WGpuTextureDescriptor texture_desc =
    WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  texture_desc.width = width;
  texture_desc.height = height;
  texture_desc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  texture_desc.usage = 
    WGPU_TEXTURE_USAGE_COPY_DST 
    | WGPU_TEXTURE_USAGE_TEXTURE_BINDING
    | WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;

  downloaded_image_args_t *args = _args;
  textures[args->index] = wgpu_device_create_texture(device, &texture_desc);

  WGpuCopyExternalImageSourceInfo src = {};
  src.source = bitmap;

  WGpuCopyExternalImageDestInfo dst = {};
  dst.texture = textures[args->index];

  wgpu_queue_copy_external_image_to_texture(
      wgpu_device_get_queue(device), &src, &dst, width, height, 1);

  WGpuSamplerDescriptor sampler_desc = 
    WGPU_SAMPLER_DESCRIPTOR_DEFAULT_INITIALIZER;
  samplers[args->index] = wgpu_device_create_sampler(device, &sampler_desc);

  WGpuBufferDescriptor model_matrix_buffer_descriptor = {};
  model_matrix_buffer_descriptor.size = sizeof(m4_t);
  model_matrix_buffer_descriptor.usage = 
    WGPU_BUFFER_USAGE_UNIFORM | WGPU_BUFFER_USAGE_COPY_DST;
  model_matrix_buffer_descriptor.mappedAtCreation = WGPU_FALSE;

  model_matrix_buffers[args->index] = 
    wgpu_device_create_buffer(device, &model_matrix_buffer_descriptor);
  
  WGpuBindGroupEntry bind_group_entries[3] = {};
  bind_group_entries[0].binding = 0;
  bind_group_entries[0].resource = wgpu_texture_create_view(textures[args->index], 0);
  bind_group_entries[1].binding = 1;
  bind_group_entries[1].resource = samplers[args->index];
  bind_group_entries[2].binding = 2;
  bind_group_entries[2].resource = model_matrix_buffers[args->index];

  per_object_bind_groups[args->index] = 
    wgpu_device_create_bind_group(
        device, 
        wgpu_pipeline_get_bind_group_layout(main_render_pipeline, 1), 
        bind_group_entries, 
        3);

  // let's try making the diagonal length constant! (2 units)
#define add_quad(v1, v2, v3, v4, i) \
  do { \
    cpu_vertex_buffer[args->index][i+0] = (picture_vertex_t){(v1), (v2_t){-1, -1}}; \
    cpu_vertex_buffer[args->index][i+1] = (picture_vertex_t){(v2), (v2_t){-1, -1}}; \
    cpu_vertex_buffer[args->index][i+2] = (picture_vertex_t){(v3), (v2_t){-1, -1}}; \
    cpu_vertex_buffer[args->index][i+3] = (picture_vertex_t){(v3), (v2_t){-1, -1}}; \
    cpu_vertex_buffer[args->index][i+4] = (picture_vertex_t){(v4), (v2_t){-1, -1}}; \
    cpu_vertex_buffer[args->index][i+5] = (picture_vertex_t){(v1), (v2_t){-1, -1}}; \
  } while (false)

#define add_border(v1, v2, v3, v4, v5, v6, v7, v8, j) \
  do { \
    /* right */ add_quad(v2, v6, v7, v3, j+0); \
    /* bottom */ add_quad(v4, v8, v7, v3, j+6); \
    /* back */ add_quad(v5, v6, v7, v8, j+12); \
    /* top */ add_quad(v1, v5, v6, v2, j+18); \
    /* front */ add_quad(v1, v2, v3, v4, j+24); \
    /* left */ add_quad(v1, v5, v8, v4, j+30); \
  } while (false)

// #define add_border_vt(v1, v2, v3, v4, v5, v6, v7, v8, j) \
//   do { \
//     /* right */ add_quad(v2, v6, v7, v3, j+0); \
//     /* back */ add_quad(v5, v6, v7, v8, j+6); \
//     /* front */ add_quad(v1, v2, v3, v4, j+12); \
//     /* left */ add_quad(v1, v5, v8, v4, j+18); \
//   } while (false)

  {
    float x0 = (float)-width, y0 = (float)height;
    float l = em(sqrt)(x0*x0 + y0*y0);
    x0 /= l, y0 /= l;
    float w = -2 * x0;
    float h = 2 * y0;
    float p = 0.05;
    float c = 0.05;

    // picture
    add_quad(
        ((v3_t){x0, y0, 0}),
        ((v3_t){x0 + w, y0, 0}),
        ((v3_t){x0 + w, y0 - h, 0}),
        ((v3_t){x0, y0 - h, 0}),
        0);

    cpu_vertex_buffer[args->index][0].uv = cpu_vertex_buffer[args->index][5].uv = (v2_t){0, 0};
    cpu_vertex_buffer[args->index][1].uv = (v2_t){1, 0};
    cpu_vertex_buffer[args->index][2].uv = cpu_vertex_buffer[args->index][3].uv = (v2_t){1, 1};
    cpu_vertex_buffer[args->index][4].uv = (v2_t){0, 1};

    // bottom
    // add_border(
    //     ((v3_t){x0 + 2 * w / 3 - p, y0 - h, p}),
    //     ((v3_t){x0 + w, y0 - h, p}),
    //     ((v3_t){x0 + w + p, y0 - h - p, p}),
    //     ((v3_t){x0 + 2 * w / 3, y0 - h - p, p}),
    //     ((v3_t){x0 + 2 * w / 3 - p, y0 - h, 0}),
    //     ((v3_t){x0 + w, y0 - h, 0}),
    //     ((v3_t){x0 + w + p, y0 - h - p, 0}),
    //     ((v3_t){x0 + 2 * w / 3, y0 - h - p, 0}),
    //     6);
    //
    // // right
    // add_border(
    //     ((v3_t){x0 + w, y0 - 2 * h / 3 + p, p}),
    //     ((v3_t){x0 + w + p, y0 - 2 * h / 3, p}),
    //     ((v3_t){x0 + w + p, y0 - h - p, p}),
    //     ((v3_t){x0 + w, y0 - h, p}),
    //     ((v3_t){x0 + w, y0 - 2 * h / 3 + p, 0}),
    //     ((v3_t){x0 + w + p, y0 - 2 * h / 3, 0}),
    //     ((v3_t){x0 + w + p, y0 - h - p, 0}),
    //     ((v3_t){x0 + w, y0 - h, 0}),
    //     42);
    //
    // // top
    // add_border(
    //     ((v3_t){x0 - p, y0 + p, p}),
    //     ((v3_t){x0 + w / 3, y0 + p, p}),
    //     ((v3_t){x0 + w / 3 + p, y0, p}),
    //     ((v3_t){x0, y0, p}),
    //     ((v3_t){x0 - p, y0 + p, 0}),
    //     ((v3_t){x0 + w / 3, y0 + p, 0}),
    //     ((v3_t){x0 + w / 3 + p, y0, 0}),
    //     ((v3_t){x0, y0, 0}),
    //     78);
    //
    // // left
    // add_border(
    //     ((v3_t){x0 - p, y0 + p, p}),
    //     ((v3_t){x0, y0, p}),
    //     ((v3_t){x0, y0 - h / 3 - p, p}),
    //     ((v3_t){x0 - p, y0 - h / 3, p}),
    //     ((v3_t){x0 - p, y0 + p, 0}),
    //     ((v3_t){x0, y0, 0}),
    //     ((v3_t){x0, y0 - h / 3 - p, 0}),
    //     ((v3_t){x0 - p, y0 - h / 3, 0}),
    //     114);
    //
    // // top cube
    // add_border(
    //     ((v3_t){x0 + w, y0 + c, c}),
    //     ((v3_t){x0 + w + c, y0 + c, c}),
    //     ((v3_t){x0 + w + c, y0, c}),
    //     ((v3_t){x0 + w, y0, c}),
    //     ((v3_t){x0 + w, y0 + c, 0}),
    //     ((v3_t){x0 + w + c, y0 + c, 0}),
    //     ((v3_t){x0 + w + c, y0, 0}),
    //     ((v3_t){x0 + w, y0, 0}),
    //     150);
    //
    // // bottom cube
    // add_border(
    //     ((v3_t){x0 - c, y0 - h, c}),
    //     ((v3_t){x0, y0 - h, c}),
    //     ((v3_t){x0, y0 - h - c, c}),
    //     ((v3_t){x0 - c, y0 - h - c, c}),
    //     ((v3_t){x0 - c, y0 - h, 0}),
    //     ((v3_t){x0, y0 - h, 0}),
    //     ((v3_t){x0, y0 - h - c, 0}),
    //     ((v3_t){x0 - c, y0 - h - c, 0}),
    //     186);
  }

#undef add_border
#undef add_quad

  WGpuBufferDescriptor vertex_buffer_descriptor = {};
  vertex_buffer_descriptor.size = sizeof(cpu_vertex_buffer[0]);
  vertex_buffer_descriptor.usage = WGPU_BUFFER_USAGE_VERTEX;
  vertex_buffer_descriptor.mappedAtCreation = WGPU_TRUE;

  WGpuBuffer vertex_buffer = wgpu_device_create_buffer(device, &vertex_buffer_descriptor);
  vertex_buffers[args->index] = vertex_buffer;

  wgpu_buffer_get_mapped_range(vertex_buffer, 0, WGPU_MAX_SIZE);
  wgpu_buffer_write_mapped_range(vertex_buffer, 0, 0, cpu_vertex_buffer[args->index], sizeof(cpu_vertex_buffer[args->index]));
  wgpu_buffer_unmap(vertex_buffer);

  (void)atomic_fetch_add(&n_textures_loaded, 1);
  free(args);
}

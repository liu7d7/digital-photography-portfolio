#include <math.h>
#include <miniprintf.h>
#include "images.h"
#include "state.h"

pic_vertex_t cpu_vb[n_texs][6];

void downloaded_image(
    WGpuImageBitmap bitmap,
    int width,
    int height,
    void *_args)
{
  downloaded_image_args_t *args = _args;
  int index = args->index;
  state_t *s = args->state;

  if (width == 0) {
    emscripten_mini_stdio_printf("error downloading image %d; w=%d, h=%d\n", index, width, height);
    goto fail;
  }

  /*--- dlimg :> s->texs[index] ---*/
  {
    WGpuTextureDescriptor tex_desc =
      WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;

    tex_desc.width = width;
    tex_desc.height = height;
    tex_desc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
    tex_desc.usage = 
      WGPU_TEXTURE_USAGE_COPY_DST 
      | WGPU_TEXTURE_USAGE_TEXTURE_BINDING
      | WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;

    s->texs[index] = wgpu_device_create_texture(s->dev, &tex_desc);

    WGpuCopyExternalImageSourceInfo src = { .source = bitmap };
    WGpuCopyExternalImageDestInfo dst = { .texture = s->texs[index] };
    wgpu_queue_copy_external_image_to_texture(
        wgpu_device_get_queue(s->dev), &src, &dst, width, height, 1);
  }

#define random() em(random)

  /*--- dlimg :> s->model_mat_ubs[index] ---*/
  WGpuBuffer mmub; 
  {
    float yaw = index / (float)n_texs * 2. * M_PI, pitch = random() * (M_PI * 0.6) - M_PI * 0.6 * 0.5;
    if (index == 0) yaw = 0, pitch = 0;

    v3_t z = {
      em(cos, yaw) * em(cos, pitch),
      em(sin, pitch),
      em(sin, yaw) * em(cos, pitch)
    };
    v3_normalize(&z);
    
    v3_t x = v3_cross(z, (v3_t){0, 1, 0});
    v3_normalize(&x);

    v3_t y = v3_cross(x, z);

    v3_t tr = v3_add(
        v3_mul((v3_t){em(cos, -yaw), random(), em(sin, -yaw)}, 4.f * random()),
        (v3_t){random(), 0.4 * (index % 2), index * 8});

    m4_t rmm = {
      x.x, y.x, z.x, tr.x,
      x.y, y.y, z.y, tr.y,
      x.z, y.z, z.z, tr.z,
      0, 0, 0, 1
    };

    s->poses[index] = (pic_pose_t){
      .yaw = yaw,
      .pitch = pitch,
      .pos = v3_add(tr, v3_mul(z, -3.25f))
    };

    WGpuBufferDescriptor model_mat_ub_desc = {};
    model_mat_ub_desc.size = sizeof(m4_t);
    model_mat_ub_desc.usage = 
      WGPU_BUFFER_USAGE_STORAGE | WGPU_BUFFER_USAGE_COPY_DST;
    model_mat_ub_desc.mappedAtCreation = WGPU_TRUE;

    mmub = wgpu_device_create_buffer(s->dev, &model_mat_ub_desc);
    s->model_mat_ubs[index] = mmub;

    wgpu_buffer_get_mapped_range(mmub, 0, WGPU_MAX_SIZE);
    wgpu_buffer_write_mapped_range(mmub, 0, 0, &rmm, sizeof(rmm));
    wgpu_buffer_unmap(mmub);
  }
  
  /*--- dlimg :> s->per_obj_bgs[index] ---*/
  {
    WGpuBindGroupEntry bind_group_entries[] = {
      {
        .binding = 0,
        .resource = wgpu_texture_create_view(s->texs[index], 0) 
      },
      { .binding = 1, .resource = s->d_samp },
      { .binding = 2, .resource = mmub }
    };

    s->per_obj_bgs[index] = 
      wgpu_device_create_bind_group(
          s->dev, 
          wgpu_render_pipeline_get_bind_group_layout(s->main_rp, 1), 
          bind_group_entries, 
          3);
  }

#undef random

#define add_quad(v1, v2, v3, v4, i) \
  do { \
    cpu_vb[index][i+0] = (pic_vertex_t){(v1), (v2_t){-1, -1}}; \
    cpu_vb[index][i+1] = (pic_vertex_t){(v2), (v2_t){-1, -1}}; \
    cpu_vb[index][i+2] = (pic_vertex_t){(v3), (v2_t){-1, -1}}; \
    cpu_vb[index][i+3] = (pic_vertex_t){(v3), (v2_t){-1, -1}}; \
    cpu_vb[index][i+4] = (pic_vertex_t){(v4), (v2_t){-1, -1}}; \
    cpu_vb[index][i+5] = (pic_vertex_t){(v1), (v2_t){-1, -1}}; \
  } while (false)

  /*--- dlimg :> cpu_vb[index] ---*/
  {
    float x0 = (float)-width, y0 = (float)height;
    float l = em(sqrt, x0*x0 + y0*y0);
    x0 /= l, y0 /= l;
    float w = -2 * x0;
    float h = 2 * y0;

    // pic
    add_quad(
        ((v3_t){x0, y0, 0}),
        ((v3_t){x0 + w, y0, 0}),
        ((v3_t){x0 + w, y0 - h, 0}),
        ((v3_t){x0, y0 - h, 0}),
        0);

    cpu_vb[index][0].uv = cpu_vb[index][5].uv = (v2_t){0, 0};
    cpu_vb[index][1].uv = (v2_t){1, 0};
    cpu_vb[index][2].uv = cpu_vb[index][3].uv = (v2_t){1, 1};
    cpu_vb[index][4].uv = (v2_t){0, 1};
  }

#undef add_quad

  /*--- dlimg : s->vbs[index] ---*/
  {
    WGpuBufferDescriptor vb_desc  = {};
    vb_desc.size = sizeof(cpu_vb[0]);
    vb_desc.usage = WGPU_BUFFER_USAGE_VERTEX;
    vb_desc.mappedAtCreation = WGPU_TRUE;

    WGpuBuffer vb  = wgpu_device_create_buffer(s->dev, &vb_desc);
    s->vbs[index] = vb;

    wgpu_buffer_get_mapped_range(vb, 0, WGPU_MAX_SIZE);
    wgpu_buffer_write_mapped_range(
        vb,
        0,
        0,
        cpu_vb[index],
        sizeof(cpu_vb[index]));

    wgpu_buffer_unmap(vb);
  }

fail:
  (void)atomic_fetch_add(&s->n_texs_loaded, 1);
  if (index + 2 >= n_texs) {
    free(args);
  } else {
    args->index += 2;
    wgpu_load_image_bitmap_from_url_async(
        tex_paths[args->index], 
        WGPU_TRUE, 
        downloaded_image, 
        args);
  }
}

#include "state.h"
#include "font.h"
#include "shader_sources.h"
#include <miniprintf.h>
#include <string.h>

void downloaded_font_image(
    WGpuImageBitmap bmp,
    int width,
    int height,
    void *user_data)
{
  state_t *s = user_data;

  if (width == 0) {
    emscripten_mini_stdio_printf("error downloading font image; w=%d, h=%d\n", index, width, height);
    return;
  }

  WGpuTextureDescriptor tex_desc =
    WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;

  tex_desc.width = width;
  tex_desc.height = height;
  tex_desc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  tex_desc.usage = 
    WGPU_TEXTURE_USAGE_COPY_DST 
    | WGPU_TEXTURE_USAGE_TEXTURE_BINDING
    | WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;

  s->font.tex = wgpu_device_create_texture(s->dev, &tex_desc);

  WGpuCopyExternalImageSourceInfo src = { .source = bmp };
  WGpuCopyExternalImageDestInfo dst = { .texture = s->font.tex };
  wgpu_queue_copy_external_image_to_texture(
      wgpu_device_get_queue(s->dev), &src, &dst, width, height, 1);
}

void downloaded_font_metadata(
    uint8_t *data,
    int size,
    void *user_data)
{
  state_t *s = user_data;
  int const expected_size = sizeof(pch_t) * 512 + sizeof(metrics_t);
  if (size != expected_size) {
    emscripten_mini_stdio_printf("size mismatch in font metdata! expected %d, got %d bytes\n", expected_size, size);
    return;
  }

  void *dst = ((void *)&s->font) + sizeof(WGpuTexture);
  memcpy(dst, data, sizeof(pch_t) * 512 + sizeof(metrics_t));
  free(data);
}

void font_new_rp(state_t *s)
{
  WGpuVertexAttribute attrs[] = {
    {.format=WGPU_VERTEX_FORMAT_FLOAT32X3, .offset=0, .shaderLocation=0},
    {.format=WGPU_VERTEX_FORMAT_FLOAT32X2, .offset=12, .shaderLocation=1}
  };

  WGpuVertexBufferLayout vb_layout = {
    .numAttributes = 2,
    .attributes = attrs,
    .arrayStride = 20
  };

  s->font_rp = state_new_render_pipeline(
      s,
      1, &vb_layout,
      font_vertex_shader_source, "main",
      font_fragment_shader_source, "main",
      0, NULL);
}


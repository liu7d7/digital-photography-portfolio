#include "state.h"
#include "font.h"
#include <miniprintf.h>
#include <string.h>

void downloaded_font_image(
    WGpuImageBitmap bmp,
    int width,
    int height,
    void *user_data)
{
  downloaded_font_args_t *args = user_data;
  state_t *s = args->s;
  font_metadata_t *target = args->dst;

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

  target->tex = wgpu_device_create_texture(s->dev, &tex_desc);
  target->tex_view = wgpu_texture_create_view(target->tex, NULL);

  WGpuCopyExternalImageSourceInfo src = { .source = bmp };
  WGpuCopyExternalImageDestInfo dst = { .texture = target->tex };
  wgpu_queue_copy_external_image_to_texture(
      wgpu_device_get_queue(s->dev), &src, &dst, width, height, 1);

  free(user_data);

  target->tex_ready = true;
}

void downloaded_font_metadata(
    uint8_t *data,
    int size,
    void *user_data)
{
  downloaded_font_args_t *args = user_data;
  font_metadata_t *target = args->dst;

  int const expected_size = sizeof(pch_t) * 512 + sizeof(metrics_t);
  if (size != expected_size) {
    emscripten_mini_stdio_printf("size mismatch in font metdata! expected %d, got %d bytes\n", expected_size, size);
    return;
  }

  void *dst = (void *)&target->mt;
  memcpy(dst, data, sizeof(pch_t) * 512 + sizeof(metrics_t));
  free(data);
  free(user_data);

  emscripten_mini_stdio_printf("%f, %f, %d, %d, %d, %d\n", target->mt.scale_to_one[0], target->mt.scale_to_one[1], target->mt.line_gap[0], target->mt.line_gap[1], target->mt.ascent[0], target->mt.ascent[1]);

  target->metrics_ready = true;
}

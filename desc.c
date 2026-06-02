#include "state.h"
#include "desc.h"
#include <string.h>

desc_set_t desc_set_new(
    state_t *s,
    int n,
    font_metadata_t *fm,
    WGpuBuffer *model_mat_ubs /* [n] */, 
    font_draw_cmd_t *cmds /* [n] */)
{
  desc_set_t out = {.n_descs = n};
  void *mem = malloc(sizeof(int) * (2 * n * 1));
  out.desc_byte_bounds = mem;
  out.desc_byte_bounds[0] = 0;

  int total_length = 0;
  for (int i = 0; i < n; i++) {
    cmds[i].len = strlen(cmds[i].text);
    total_length += cmds[i].len;
  }

  typedef struct vertex_t {
    v3_t pos;
    v2_t uv;
  } vertex_t;

  vertex_t *verts = malloc(sizeof(vertex_t) * total_length * 6);
  int end = 0;
  for (int i = 0; i < n; i++) {
    v3_t pos = cmds[i].pos;
    int font = 0;
    int len = cmds[i].len;
    char const *text = cmds[i].text;
    float scale = cmds[i].scale;
    for (char const *cp = text; cp != text + len; cp++) { 
      char c = *cp;
      if (c == '\\') {
        if (cp == text + len - 1) break;
        c = *(++cp);
        if (c == '\\') {
          goto regular_character;
        } else if (c == 'b') {
          font = 1;
          goto escape_charater;
        } else if (c == 'r') {
          font = 0;
          goto escape_charater;
        } else if (c == 'n') {
          pos.x = cmds[i].pos.x;
          pos.y += fm->mt.ascent_in_pixels[font];
        } else {
          continue;
        }

escape_charater:
        if (cp == text + len - 1) break;
        c = *(++cp);
      }

regular_character: {}
      pch_t pch = fm->pch[font * 256 + c];
      float px0 = pos.x + pch.xoff * scale;
      float py0 = pos.y + pch.yoff * scale;
      float px1 = pos.x + pch.xoff2 * scale;
      float py1 = pos.y + pch.yoff2 * scale;
      verts[end++] = (vertex_t){
        .pos = {px0, py0}, .
          uv = {pch.x0 / 8192.f, pch.y0 / 8192.f}
      };

      verts[end++] = (vertex_t){
        .pos = {px1, py0}, 
        .uv = {pch.x1 / 8192.f, pch.y0 / 8192.f}
      };

      verts[end++] = (vertex_t){
        .pos = {px1, py1}, 
        .uv = {pch.x1 / 8192.f, pch.y1 / 8192.f}
      };

      verts[end++] = (vertex_t){
        .pos = {px1, py1}, 
        .uv = {pch.x1 / 8192.f, pch.y1 / 8192.f}
      };

      verts[end++] = (vertex_t){
        .pos = {px0, py1}, 
        .uv = {pch.x0 / 8192.f, pch.y1 / 8192.f}
      };

      verts[end++] = (vertex_t){
        .pos = {px0, py0}, 
        .uv = {pch.x0 / 8192.f, pch.y0 / 8192.f}
      };
      pos.x += pch.xadvance * scale;
    }
    
    out.desc_byte_bounds[cmds[i].desc_id] = end * sizeof(vertex_t);
  }

  out.bgs = mem + sizeof(int) * n + 1;
  for (int i = 0; i < n; i++) {
    WGpuBindGroupEntry bges[] = {
      {.binding=0, .resource=fm->tex},
      {.binding=1, .resource=s->d_samp},
      {.binding=2, .resource=model_mat_ubs[cmds[i].desc_id]}
    };

    out.bgs[i] = wgpu_device_create_bind_group(s->dev, s->font_rp, bges, 3);
  }

  WGpuBufferDescriptor vb_desc = {
    .mappedAtCreation = WGPU_TRUE,
    .size = end * sizeof(vertex_t),
    .usage = WGPU_BUFFER_USAGE_VERTEX
  };

  out.vb = wgpu_device_create_buffer(s->dev, &vb_desc);
  wgpu_buffer_get_mapped_range(out.vb, 0, WGPU_MAX_SIZE);
  wgpu_buffer_write_mapped_range(out.vb, 0, 0, verts, vb_desc.size);
  wgpu_buffer_unmap(out.vb);

  return out;
}


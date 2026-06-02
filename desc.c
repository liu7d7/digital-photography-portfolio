#include "state.h"
#include <miniprintf.h>
#include "desc.h"
#include <string.h>

desc_set_t desc_set_new(
    state_t *s,
    font_metadata_t *fm,
    int n_unique_ids,
    int n,
    WGpuBuffer *model_mat_ubs /* [n] */, 
    font_draw_cmd_t *cmds /* [n] */)
{
  desc_set_t out = {.n_unique_ids = n};
  out.desc_byte_bounds = malloc(sizeof(int) * (n_unique_ids + 1));
  out.desc_vert_bounds = malloc(sizeof(int) * (n_unique_ids + 1));
  out.desc_vert_bounds[0] = out.desc_byte_bounds[0] = 0;

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
    float scale = cmds[i].scale * fm->mt.scale_to_one[font];
    int len = cmds[i].len;
    char const *text = cmds[i].text;
    float max_pos_x = pos.x;
    int begin = end;
    for (char const *cp = text; cp != text + len; cp++) { 
      uint8_t c = *cp;
      if (c == '\\') {
        if (cp == text + len - 1) break;
        c = *(++cp);
        if (c == '\\') {
          goto regular_character;
        } else if (c == 'b') {
          font = 1;
        } else if (c == 'r') {
          font = 0;
        } else if (c == 'n') {
          float width = pos.x - cmds[i].pos.x;
          if (cmds[i].justify) {
            for (int j = begin; j < end; j++) {
              verts[j].pos.x -= width / cmds[i].justify;
            }

            begin = end;
          }

          pos.x = cmds[i].pos.x;
          pos.y += fm->mt.ascent[font] / 3. * scale;
        } else {
          continue;
        }

        if (cp == text + len - 1) break;
        scale = cmds[i].scale * fm->mt.scale_to_one[font];
        continue;
      }

regular_character: {}
      pch_t pch = fm->pch[font * 256 + c];
      float px0 = pos.x + pch.xoff * scale;
      float py0 = pos.y + pch.yoff * scale;
      float px1 = pos.x + pch.xoff2 * scale;
      float py1 = pos.y + pch.yoff2 * scale;

      verts[end++] = (vertex_t){
        .pos = {px0, py0}, 
        .uv = {pch.x0 / 8192.f, pch.y0 / 8192.f}
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
      max_pos_x = fmaxf(pos.x, max_pos_x);
    }

    out.desc_byte_bounds[cmds[i].desc_id + 1] = end * sizeof(vertex_t);
    out.desc_vert_bounds[cmds[i].desc_id + 1] = end;

    float width = pos.x - cmds[i].pos.x;
    if (cmds[i].justify) {
      for (int j = begin; j < end; j++) {
        verts[j].pos.x -= width / cmds[i].justify;
      }
    }
  }

  for (int i = 0; i < n_unique_ids; i++) {
    emscripten_mini_stdio_printf("dsn: %d\n", out.desc_byte_bounds[i + 1]);
  }

  out.bgs = malloc(sizeof(WGpuBindGroup) * n_unique_ids);
  for (int i = 0; i < n_unique_ids; i++) {
    WGpuBindGroupEntry bges[] = {
      {.binding=0, .resource=fm->tex},
      {.binding=1, .resource=s->d_samp},
      {.binding=2, .resource=model_mat_ubs[i]}
    };

    out.bgs[i] = wgpu_device_create_bind_group(
        s->dev,
        wgpu_render_pipeline_get_bind_group_layout(s->main_rp, 1),
        bges,
        3);
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


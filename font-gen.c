#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_rect_pack.h"
#include "stb_truetype.h"
#include "stb_image_write.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

static uint8_t *read_bin_file(char const *path)
{
  FILE *f = fopen(path, "rb");
  if (!f) {
    fclose(f);
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  rewind(f);

  uint8_t *buf = malloc(size);
  if (!fread(buf, size, 1, f)) {
    fclose(f);
    free(buf);
    return NULL;
  }

  fclose(f);
  return buf;
}

#define N 2

int main() 
{
  uint8_t *fb[N] = {
    read_bin_file("fdl.otf"),
    read_bin_file("fdb.otf"),
  };

  assert(fb[0] != NULL);
  assert(fb[1] != NULL);

  uint8_t *px = malloc(8192 * 8192);
  stbtt_pack_context pc = {};
  stbtt_PackBegin(&pc, px, 8192, 8192, 0, 1, NULL);
  stbtt_PackSetOversampling(&pc, 2, 2);

  stbtt_fontinfo fi[N];
  struct metrics {
    int ascent[2];
    float ascent_in_pixels[2];
  } mt;

  size_t pch_size = sizeof(stbtt_packedchar) * 512 + sizeof(struct metrics);
  void *pch = malloc(pch_size);
  for (int i = 0; i < N; i++) {
    (void)stbtt_PackFontRange(&pc, fb[i], 0, 480, 0, 256, pch + sizeof(struct metrics) + i * 256 * sizeof(stbtt_packedchar));
    (void)stbtt_InitFont(&fi[i], fb[i], 0);
    stbtt_GetFontVMetrics(&fi[i], &mt.ascent[i], NULL, NULL); 
    mt.ascent_in_pixels[i] = (float)mt.ascent[i] * stbtt_ScaleForMappingEmToPixels(&fi[i], 32); 
  }

  memcpy(pch + sizeof(stbtt_packedchar) * 512, &mt, sizeof(mt));
  stbtt_PackEnd(&pc);

  stbi_write_png("faune.png", 8192, 8192, 1, px, 0);

  FILE *f = fopen("faune.dat", "wb");
  fwrite(pch, pch_size, 1, f);
  fclose(f);
}

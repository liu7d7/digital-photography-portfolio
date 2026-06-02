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
    read_bin_file("ip0.ttf"),
    read_bin_file("ip1.ttf"),
  };

  assert(fb[0] != NULL);
  assert(fb[1] != NULL);

  uint8_t *px = malloc(8192 * 8192);
  stbtt_pack_context pc = {};
  stbtt_PackBegin(&pc, px, 8192, 8192, 0, 1, NULL);
  stbtt_PackSetOversampling(&pc, 2, 2);

  stbtt_fontinfo fi[N];
  struct metrics {
    int line_gap[2];
    int ascent[2];
    float scale_to_one[2];
  } mt;

  size_t pch_size = sizeof(stbtt_packedchar) * 512 + sizeof(struct metrics);
  void *pch = malloc(pch_size);
  for (int i = 0; i < N; i++) {
    (void)stbtt_PackFontRange(&pc, fb[i], 0, STBTT_POINT_SIZE(320), 0, 256, pch + sizeof(struct metrics) + i * 256 * sizeof(stbtt_packedchar));
    (void)stbtt_InitFont(&fi[i], fb[i], 0);
    stbtt_GetFontVMetrics(&fi[i], &mt.ascent[i], NULL, &mt.line_gap[i]); 
    mt.scale_to_one[i] = stbtt_ScaleForMappingEmToPixels(&fi[i], 1); 

    printf("%f, %d, %d\n", mt.scale_to_one[i], mt.line_gap[i], mt.ascent[i]);
  }

  memcpy(pch, &mt, sizeof(mt));
  stbtt_PackEnd(&pc);

  stbi_write_png("font.png", 8192, 8192, 1, px, 0);

  FILE *f = fopen("font.dat", "wb");
  fwrite(pch, pch_size, 1, f);
  fclose(f);
}

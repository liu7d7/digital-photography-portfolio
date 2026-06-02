#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void download_binary_file(char const *url, void (*callback)(uint8_t *data, int size, void *userData), void *userData);

#ifdef __cplusplus
}
#endif

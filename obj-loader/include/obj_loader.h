#ifndef OBJ_LOADER
#define OBJ_LOADER 1

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Floats {
  float *ptr;
  size_t len;
};

void load_obj(const char *path);

#ifdef __cplusplus
}
#endif

#endif // OBJ_LOADER

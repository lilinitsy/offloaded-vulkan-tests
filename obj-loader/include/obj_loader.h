#ifndef OBJ_LOADER
#define OBJ_LOADER 1

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
namespace obj {
extern "C" {
#endif

struct F32s {
  const float *ptr;
  size_t len;
};

struct U32s {
  const uint32_t *ptr;
  size_t len;
};

struct Model {
  F32s positions;
  F32s normals;
  F32s texcoords;
  U32s indices;
  size_t material_id;
  uint8_t has_material;
  const char *name;
};

struct Models {
  const struct Model *ptr;
  size_t len;
};

struct ModelsAndMaterials {
  Models models;
};

struct ModelsAndMaterials load_obj(const char *path);

#ifdef __cplusplus
}
}
#endif

#endif // OBJ_LOADER

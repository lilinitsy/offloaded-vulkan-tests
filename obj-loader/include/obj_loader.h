#ifndef OBJ_LOADER
#define OBJ_LOADER 1

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
namespace obj
{
	extern "C"
	{
#endif

		struct F32s
		{
			const float *ptr;
			size_t len;
		};

		struct U32s
		{
			const uint32_t *ptr;
			size_t len;
		};

		struct Model
		{
			struct F32s positions;
			struct F32s normals;
			struct F32s texcoords;
			struct U32s indices;
			size_t material_id;
			uint8_t has_material;
			const char *name;
		};

		struct Models
		{
			const struct Model *ptr;
			size_t len;
		};

		struct Material
		{
			const char *diffuse_texture;
		};

		struct Materials
		{
			const struct Material *ptr;
			size_t len;
		};

		struct ModelsAndMaterials
		{
			struct Models models;
			struct Materials materials;
		};

		struct ModelsAndMaterials load_obj(const char *path);

#ifdef __cplusplus
	}
}
#endif

#endif // OBJ_LOADER

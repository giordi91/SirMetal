#pragma once
#include <vector>
#include <simd/simd.h>
#include "SirMetal/resources/handle.h"

namespace SirMetal {
struct EngineContext;

struct Model
{
  simd_float4x4 matrix;
  MeshHandle mesh;

};
struct GLTFAsset {
  std::vector<Model> models;
};

enum GLTFLoadFlags {
  GLTF_LOAD_FLAGS_NONE = 0,
  GLTF_LOAD_FLAGS_FLATTEN_HIERARCHY
};

bool loadGLTF(EngineContext* context,const char *path, GLTFAsset &outAsset,
              GLTFLoadFlags flags = GLTF_LOAD_FLAGS_NONE);

} // namespace SirMetal
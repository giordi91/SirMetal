#pragma once
#include "SirMetal/resources/handle.h"
#include <simd/simd.h>
#include <vector>
#include <string>

namespace SirMetal {
struct EngineContext;

struct GLTFMaterial {
  std::string name;
  simd_float4 colorFactors;
  TextureHandle colorTexture;
  bool doubleSided;

};

struct Model {
  simd_float4x4 matrix;
  MeshHandle mesh;
  GLTFMaterial material;
};
struct GLTFAsset {
  std::vector<Model> models;
};

enum GLTFLoadFlags {
  GLTF_LOAD_FLAGS_NONE = 0,
  GLTF_LOAD_FLAGS_FLATTEN_HIERARCHY
};

bool loadGLTF(EngineContext *context, const char *path, GLTFAsset &outAsset,
              GLTFLoadFlags flags = GLTF_LOAD_FLAGS_NONE);

} // namespace SirMetal
#pragma once
#include "SirMetal/resources/handle.h"
#include <simd/simd.h>
#include <string>
#include <vector>

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

enum GLTFLoadFlags : uint32_t {
  GLTF_LOAD_FLAGS_NONE = 0,
  GLTF_LOAD_FLAGS_FLATTEN_HIERARCHY = 1,
  GLTF_LOAD_FLAGS_GENERATE_LIGHT_MAP_UVS = 2,
};


struct GLTFLoadOptions
{
  uint32_t flags = GLTF_LOAD_FLAGS_NONE; //GLTFLoadFlags
  uint32_t lightMapSize = 2048;
};
bool loadGLTF(EngineContext *context, const char *path, GLTFAsset &outAsset,
              const GLTFLoadOptions& options);

}// namespace SirMetal
#pragma once

#include <vector>
#include <stdint.h>
#include <string>

#include "SirMetal/core/core.h"

namespace SirMetal
{
enum class LOAD_MESH_TYPE {
  INVALID =0,
  GLTF_MESH
};

enum MESH_ATTRIBUTE_TYPE {
  MESH_ATTRIBUTE_TYPE_POSITION = 0,
  MESH_ATTRIBUTE_TYPE_NORMAL = 1,
  MESH_ATTRIBUTE_TYPE_UV = 2,
  MESH_ATTRIBUTE_TYPE_TANGENT = 3,
  MESH_ATTRIBUTE_TYPE_COUNT = 4
};

// list of mesh attributes we want to extract from the gtlf file
static char const *MESH_ATTRIBUTES[MESH_ATTRIBUTE_TYPE_COUNT] = {
    "POSITION",
    "NORMAL",
    "TEXCOORD_0",
    "TANGENT",
};

static constexpr float
    MESH_ATTRIBUTES_COMPONENT_FILLER[MESH_ATTRIBUTE_TYPE_COUNT] = {1, 0, -1, 0};

struct MeshLoadResult {
  std::vector<float> vertices;
  std::vector<uint32_t> indices;
  MemoryRange ranges[4];
  float m_boundingBox[6];
  std::string name;
};
}

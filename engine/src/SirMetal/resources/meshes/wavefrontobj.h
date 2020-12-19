#pragma once

#include <vector>
#include "SirMetal/core/core.h"
#include "SirMetal/resources/meshes/meshLoadType.h"

namespace SirMetal {
struct Vertex {
  float vx, vy, vz, vw;
  float nx, ny, nz, nw;
  float tu, tv, pad1, pad2;
};

bool loadMeshObj(MeshLoadResult &result, const char *path);
}



#pragma once

#include "SirMetal/resources/meshes/meshLoadType.h"

namespace SirMetal {
bool loadGltfMesh(MeshLoadResult &outMesh, const void *gltfMesh);
}
#pragma once

#include "SirMetal/resources/resourceTypes.h"

namespace SirMetal {
bool loadGltfMesh(MeshLoadResult &outMesh, const void *gltfMesh, const void* options);
}
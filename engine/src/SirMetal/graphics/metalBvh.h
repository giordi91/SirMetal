#pragma once

#include <Metal/Metal.h>
#include <vector>
#include "SirMetal/resources/resourceTypes.h"


namespace SirMetal {

struct EngineContext;

namespace graphics {
struct MetalBVH {
  id<MTLBuffer> instanceBuffer;
  std::vector<id> primitiveAccelerationStructures;
  id instanceAccelerationStructure;
};

void buildMultiLevelBVH(EngineContext *context, std::vector<Model> models,
                        MetalBVH &outBvh);

}}// namespace SirMetal::graphics
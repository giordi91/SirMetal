#include "SirMetal/graphics/metalBvh.h"

#include "SirMetal/engine.h"
#include "SirMetal/graphics/renderingContext.h"
#include "SirMetal/resources/resourceTypes.h"
#include "SirMetal/resources/meshes/meshManager.h"
#include <Metal/Metal.h>


namespace SirMetal::graphics {

static id buildPrimitiveAccelerationStructure( EngineContext* context,
        MTLAccelerationStructureDescriptor *descriptor) {
  id<MTLDevice> device = context->m_renderingContext->getDevice();
  id<MTLCommandQueue> queue = context->m_renderingContext->getQueue();
  // Create and compact an acceleration structure, given an acceleration structure descriptor.
  // Query for the sizes needed to store and build the acceleration structure.
  MTLAccelerationStructureSizes accelSizes =
  [device accelerationStructureSizesWithDescriptor:descriptor];

  // Allocate an acceleration structure large enough for this descriptor. This doesn't actually
  // build the acceleration structure, just allocates memory.
  id<MTLAccelerationStructure> accelerationStructure =
  [device newAccelerationStructureWithSize:accelSizes.accelerationStructureSize];

  // Allocate scratch space used by Metal to build the acceleration structure.
  // Use MTLResourceStorageModePrivate for best performance since the sample
  // doesn't need access to buffer's contents.
  id<MTLBuffer> scratchBuffer =
  [device newBufferWithLength:accelSizes.buildScratchBufferSize
  options:MTLResourceStorageModePrivate];

  // Create a command buffer which will perform the acceleration structure build
  id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];

  // Create an acceleration structure command encoder.
  id<MTLAccelerationStructureCommandEncoder> commandEncoder =
  [commandBuffer accelerationStructureCommandEncoder];

  // Allocate a buffer for Metal to write the compacted accelerated structure's size into.
  id<MTLBuffer> compactedSizeBuffer =
  [device newBufferWithLength:sizeof(uint32_t)
  options:MTLResourceStorageModeShared];

  // Schedule the actual acceleration structure build
  [commandEncoder buildAccelerationStructure:accelerationStructure
  descriptor:descriptor
  scratchBuffer:scratchBuffer
  scratchBufferOffset:0];

  // Compute and write the compacted acceleration structure size into the buffer. You
  // must already have a built accelerated structure because Metal determines the compacted
  // size based on the final size of the acceleration structure. Compacting an acceleration
  // structure can potentially reclaim significant amounts of memory since Metal must
  // create the initial structure using a conservative approach.

  [commandEncoder writeCompactedAccelerationStructureSize:accelerationStructure
  toBuffer:compactedSizeBuffer
  offset:0];

  // End encoding and commit the command buffer so the GPU can start building the
  // acceleration structure.
  [commandEncoder endEncoding];

  [commandBuffer commit];

  // The sample waits for Metal to finish executing the command buffer so that it can
  // read back the compacted size.

  // Note: Don't wait for Metal to finish executing the command buffer if you aren't compacting
  // the acceleration structure, as doing so requires CPU/GPU synchronization. You don't have
  // to compact acceleration structures, but you should when creating large static acceleration
  // structures, such as static scene geometry. Avoid compacting acceleration structures that
  // you rebuild every frame, as the synchronization cost may be significant.

  [commandBuffer waitUntilCompleted];

  uint32_t compactedSize = *(uint32_t *) compactedSizeBuffer.contents;

  // Allocate a smaller acceleration structure based on the returned size.
  id<MTLAccelerationStructure> compactedAccelerationStructure =
  [device newAccelerationStructureWithSize:compactedSize];

  // Create another command buffer and encoder.
  commandBuffer = [queue commandBuffer];

  commandEncoder = [commandBuffer accelerationStructureCommandEncoder];

  // Encode the command to copy and compact the acceleration structure into the
  // smaller acceleration structure.
  [commandEncoder copyAndCompactAccelerationStructure:accelerationStructure
  toAccelerationStructure:compactedAccelerationStructure];

  // End encoding and commit the command buffer. You don't need to wait for Metal to finish
  // executing this command buffer as long as you synchronize any ray-intersection work
  // to run after this command buffer completes. The sample relies on Metal's default
  // dependency tracking on resources to automatically synchronize access to the new
  // compacted acceleration structure.
  [commandEncoder endEncoding];
  [commandBuffer commit];
  [commandBuffer waitUntilCompleted];

  return compactedAccelerationStructure;
};

void buildMultiLevelBVH(EngineContext *context, std::vector<Model> models,
                        MetalBVH &outBvh) {
  id<MTLDevice> device = context->m_renderingContext->getDevice();
  MTLResourceOptions options = MTLResourceStorageModeManaged;

  int modelCount = models.size();

  //TODO do i need the instance descriptor at all?
  //TODO right now we are not taking into account the concept of instance vs actual mesh, to do so we might need some extra
  //information in the gltf loader
  // Allocate a buffer of acceleration structure instance descriptors. Each descriptor represents
  // an instance of one of the primitive acceleration structures created above, with its own
  // transformation matrix.
  outBvh.instanceBuffer =
          [device newBufferWithLength:sizeof(MTLAccelerationStructureInstanceDescriptor) *
                                      modelCount
                              options:options];

  auto *instanceDescriptors =
          (MTLAccelerationStructureInstanceDescriptor *) outBvh.instanceBuffer.contents;

  // Create a primitive acceleration structure for each piece of geometry in the scene.
  for (NSUInteger i = 0; i < modelCount; ++i) {
    const SirMetal::Model &mesh = models[i];

    MTLAccelerationStructureTriangleGeometryDescriptor *geometryDescriptor =
            [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];

    const SirMetal::MeshData *meshData = context->m_meshManager->getMeshData(mesh.mesh);
    geometryDescriptor.vertexBuffer = meshData->vertexBuffer;
    geometryDescriptor.vertexStride = sizeof(float) * 4;
    geometryDescriptor.triangleCount = meshData->primitivesCount / 3;
    geometryDescriptor.indexBuffer = meshData->indexBuffer;
    geometryDescriptor.indexType = MTLIndexTypeUInt32;
    geometryDescriptor.indexBufferOffset = 0;
    geometryDescriptor.vertexBufferOffset = 0;

    // Assign each piece of geometry a consecutive slot in the intersection function table.
    //geometryDescriptor.intersectionFunctionTableOffset = i;

    // Create a primitive acceleration structure descriptor to contain the single piece
    // of acceleration structure geometry.
    MTLPrimitiveAccelerationStructureDescriptor *accelDescriptor =
            [MTLPrimitiveAccelerationStructureDescriptor descriptor];

    accelDescriptor.geometryDescriptors = @[geometryDescriptor];

    // Build the acceleration structure.
    id<MTLAccelerationStructure> accelerationStructure =
            buildPrimitiveAccelerationStructure(context,accelDescriptor);

    // Add the acceleration structure to the array of primitive acceleration structures.
    //[primitiveAccelerationStructures addObject:accelerationStructure];
    outBvh.primitiveAccelerationStructures.push_back(accelerationStructure);

    // Map the instance to its acceleration structure.
    instanceDescriptors[i].accelerationStructureIndex = i;

    // Mark the instance as opaque if it doesn't have an intersection function so that the
    // ray intersector doesn't attempt to execute a function that doesn't exist.
    //instanceDescriptors[instanceIndex].options = instance.geometry.intersectionFunctionName == nil ? MTLAccelerationStructureInstanceOptionOpaque : 0;
    instanceDescriptors[i].options = 0;

    // Metal adds the geometry intersection function table offset and instance intersection
    // function table offset together to determine which intersection function to execute.
    // The sample mapped geometries directly to their intersection functions above, so it
    // sets the instance's table offset to 0.
    instanceDescriptors[i].intersectionFunctionTableOffset = 0;

    // Set the instance mask, which the sample uses to filter out intersections between rays
    // and geometry. For example, it uses masks to prevent light sources from being visible
    // to secondary rays, which would result in their contribution being double-counted.
    instanceDescriptors[i].mask = (uint32_t) 0xFFFFFFFF;

    // Copy the first three rows of the instance transformation matrix. Metal assumes that
    // the bottom row is (0, 0, 0, 1).
    // This allows instance descriptors to be tightly packed in memory.
    for (int column = 0; column < 4; column++)
      for (int row = 0; row < 3; row++)
        instanceDescriptors[i].transformationMatrix.columns[column][row] =
                mesh.matrix.columns[column][row];
  }

  //update the buffer as modified
  [outBvh.instanceBuffer didModifyRange:NSMakeRange(0, outBvh.instanceBuffer.length)];

  // Create an instance acceleration structure descriptor.
  MTLInstanceAccelerationStructureDescriptor *accelDescriptor =
          [MTLInstanceAccelerationStructureDescriptor descriptor];

  //TODO temp
  NSArray *myArray = [NSArray arrayWithObjects:outBvh.primitiveAccelerationStructures.data()
                                         count:modelCount];
  accelDescriptor.instancedAccelerationStructures = myArray;
  accelDescriptor.instanceCount = modelCount;
  accelDescriptor.instanceDescriptorBuffer = outBvh.instanceBuffer;

  // Finally, create the instance acceleration structure containing all of the instances
  // in the scene.
  outBvh.instanceAccelerationStructure = buildPrimitiveAccelerationStructure(context,accelDescriptor);
}


}// namespace SirMetal::graphics

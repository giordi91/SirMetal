
#include "SirMetal/resources/meshes/meshManager.h"
#import "SirMetal/resources/meshes/wavefrontobj.h"
#import <Metal/Metal.h>
#include <SirMetal/io/file.h>

namespace SirMetal {
MeshHandle MeshManager::loadMesh(const std::string &path) {

  const std::string extString = getFileExtension(path);
  FILE_EXT ext = getFileExtFromStr(extString);
  switch (ext) {
  case FILE_EXT::OBJ: {
    return processObjMesh(path);
  }
  default: {
    printf("Unsupported mesh extension %s", extString.c_str());
  }
  }

  return MeshHandle{};
}

id<MTLBuffer> createGPUOnlyBuffer(id<MTLDevice> currDevice,
                                  id<MTLCommandQueue> queue, void *data,
                                  size_t sizeInByte) {
  id<MTLBuffer> buffer =
      [currDevice newBufferWithBytes:data
                              length:sizeInByte
                             options:MTLResourceStorageModePrivate];
  return buffer;
}

MeshHandle MeshManager::processObjMesh(const std::string &path) {

  MeshObj result;
  loadMeshObj(result, path.c_str());

  id<MTLDevice> currDevice = m_device;
  // TODO convert to a general buffer manager
  id<MTLBuffer> vertexBuffer =
      createGPUOnlyBuffer(currDevice, m_queue, result.vertices.data(),
                          sizeof(Vertex) * result.vertices.size());
  [vertexBuffer setLabel:@"Vertices"];

  id<MTLBuffer> indexBuffer =
      createGPUOnlyBuffer(currDevice, m_queue, result.indices.data(),
                          result.indices.size() * sizeof(uint32_t));
  [indexBuffer setLabel:@"Indices"];
  uint32_t index = m_meshCounter++;

  std::string meshName = getFileName(path);
  MeshData data{
      vertexBuffer,
      indexBuffer,
      {result.ranges[0], result.ranges[1], result.ranges[2], result.ranges[3]},
      static_cast<uint32_t>(result.indices.size()),
      std::move(meshName),
      matrix_float4x4_translation(vector_float3{0, 0, 0})};
  m_handleToMesh[index] = data;
  const std::string fileName = getFileName(path);
  auto handle = getHandle<MeshHandle>(index);
  m_nameToHandle[fileName] = handle.handle;
  return handle;
}
void MeshManager::cleanup() {}
} // namespace SirMetal

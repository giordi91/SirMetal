
#include "SirMetal/resources/meshes/meshManager.h"
#import "SirMetal/resources/meshes/wavefrontobj.h"
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
    assert(0);
  }
  }

  return MeshHandle{};
}

MeshHandle MeshManager::processObjMesh(const std::string &path) {

  assert(fileExists(path));
  MeshObj result;
  loadMeshObj(result, path.c_str());

  std::string meshName = getFileName(path);

  BufferHandle vhandle = m_allocator.allocate(
      sizeof(float) * result.vertices.size(), (meshName + "Vertices").c_str(),
      BUFFER_FLAG_GPU_ONLY, result.vertices.data());
  id vertexBuffer = m_allocator.getBuffer(vhandle);

  BufferHandle ihandle = m_allocator.allocate(
      result.indices.size() * sizeof(uint32_t), (meshName + "Indices").c_str(),
      BUFFER_FLAG_GPU_ONLY, result.indices.data());
  id indexBuffer = m_allocator.getBuffer(ihandle);

  uint32_t index = m_meshCounter++;
  MeshData data{
      vertexBuffer,
      indexBuffer,
      std::move(meshName),
      matrix_float4x4_translation(vector_float3{0, 0, 0}),
      {result.ranges[0], result.ranges[1], result.ranges[2], result.ranges[3]},
      static_cast<uint32_t>(result.indices.size()),
      vhandle,
      ihandle};

  m_handleToMesh[index] = data;
  const std::string fileName = getFileName(path);
  auto handle = getHandle<MeshHandle>(index);
  m_nameToHandle[fileName] = handle.handle;
  return handle;
}
void MeshManager::cleanup() {}
} // namespace SirMetal

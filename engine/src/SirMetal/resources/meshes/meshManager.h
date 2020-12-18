#pragma once

#include "SirMetal/resources/handle.h"
#import <objc/objc.h>
#import <string>
#import <unordered_map>

#import "SirMetal/core/core.h"
#include "SirMetal/core/mathUtils.h"
#include "SirMetal/core/memory/gpu/GPUMemoryAllocator.h"
#import "meshManager.h"

struct cgltf_mesh;

namespace SirMetal {
// TODO: temp public, we will need to build abstraction to render
// this data potentially without the need to extract it from here
// this is just an intermediate step
struct MeshData {
  id vertexBuffer;
  id indexBuffer;
  std::string name;
  // Temp model matrix to allow manipulation
  matrix_float4x4 modelMatrix;
  MemoryRange ranges[4];
  uint32_t primitivesCount;
  BufferHandle m_vertexHandle;
  BufferHandle m_indexHandle;
  float m_boundingBox[6]{};
};

class MeshManager {
public:
  MeshHandle loadMesh(const std::string &path);
  MeshHandle loadMesh(const cgltf_mesh* mesh);

  void initialize(id device, id queue) {
    m_allocator.initialize(device,queue);
    m_device = device;
    m_queue = queue;
  };
  const MeshHandle getHandleFromName(const std::string &name) const {
    auto found = m_nameToHandle.find(name);
    if (found != m_nameToHandle.end()) {
      return {found->second};
    }
    return {};
  }

  const MeshData *getMeshData(MeshHandle handle) const {
    assert(getTypeFromHandle(handle) == HANDLE_TYPE::MESH);
    uint32_t index = getIndexFromHandle(handle);
    auto found = m_handleToMesh.find(index);
    if (found != m_handleToMesh.end()) {
      return &found->second;
    }
    return nullptr;
  }

  void cleanup();

private:
  id m_device;
  id m_queue;
  std::unordered_map<uint32_t, MeshData> m_handleToMesh;
  std::unordered_map<std::string, uint32_t> m_nameToHandle;

  SirMetal::MeshHandle processObjMesh(const std::string &path);

  uint32_t m_meshCounter = 1;
  GPUMemoryAllocator m_allocator;
};

} // namespace SirMetal


#include "SirMetal/resources/meshes/meshManager.h"
#import "SirMetal/resources/meshes/wavefrontobj.h"
#include <SirMetal/io/file.h>
#include <cgltf/cgltf.h>

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
enum MESH_ATTRIBUTE_TYPE {
  MESH_ATTRIBUTE_TYPE_POSITION = 0,
  MESH_ATTRIBUTE_TYPE_NORMAL = 1,
  MESH_ATTRIBUTE_TYPE_UV = 2,
  MESH_ATTRIBUTE_TYPE_TANGENT = 3,
  MESH_ATTRIBUTE_TYPE_COUNT = 4

};

static cgltf_size component_size(cgltf_component_type component_type) {
  switch (component_type) {
  case cgltf_component_type_r_8:
  case cgltf_component_type_r_8u:
    return 1;
  case cgltf_component_type_r_16:
  case cgltf_component_type_r_16u:
    return 2;
  case cgltf_component_type_r_32u:
  case cgltf_component_type_r_32f:
    return 4;
  case cgltf_component_type_invalid:
  default:
    return 0;
  }
}

// list of mesh attributes we want to extract from the gtlf file
static constexpr char *MESH_ATTRIBUTES[MESH_ATTRIBUTE_TYPE_COUNT] = {
    "POSITION",
    "NORMAL",
    "TEXCOORD_0",
    "TANGENT",
};

static constexpr float
    MESH_ATTRIBUTES_COMPONENT_FILLER[MESH_ATTRIBUTE_TYPE_COUNT] = {1, 0, -1, 0};

MeshHandle MeshManager::loadMesh(const cgltf_mesh *mesh) {
  // assuming primitive count 1
  assert(mesh->primitives_count == 1);
  cgltf_primitive &prim = mesh->primitives[0];

  int uniqueVerticesCount = -1;
  // we can't simply throw memory into a buffer and hope it works. we are going
  // to create views for the same buffer, each view will point to a different
  // part of the buffer, for example a view will only refer to the position of
  // the mesh and so on.As such the data needs to be correctly aligned for
  // example normals appear after positions, the start of the normals data needs
  // to be aligned to the requested boundary if this is not the case we will add
  // some padding.
  constexpr uint32_t aligmentInFloats = 64;

  // here we accumulate all the extra data we added due to alignment
  int aligmentOffsets = 0;

  // for our mesh to be normalized to what the engine expects we require 4
  // attributes pos,normals, uvs and tangents, we look for such attributes in
  // the Gltf
  bool found = false;
  int foundIdx = 0;

  MeshData outMesh{};
  std::vector<float> meshData;

  for (int attrIdx = 0; attrIdx < MESH_ATTRIBUTE_TYPE_COUNT; ++attrIdx) {
    const char *attribute = MESH_ATTRIBUTES[attrIdx];
    for (int a = 0; a < prim.attributes_count; ++a) {
      const cgltf_attribute &primAttr = prim.attributes[a];
      found = strcmp(primAttr.name, attribute) == 0;
      if (found) {
        foundIdx = a;
        break;
      }
    }

    // if attribute is not found we patch in dummy data
    if (!found) {
      // we just make sure we know how many unique vertices we have
      // this should never trigger because if we don't have position is useless
      // data
      if (strcmp(attribute, MESH_ATTRIBUTES[MESH_ATTRIBUTE_TYPE_POSITION]) ==
          0) {
        printf("[ERROR] Position mesh attribute is empty, cannot be filled "
               "with zeroes");
        return {};
      }

      printf("[WARN] Could not find %s attribute in gltf file, filling with "
             "zeroes",
             MESH_ATTRIBUTES[attrIdx]);
      // TODO properly implement attribute pacthign
      assert(0);
      continue;
    }

    // gltf maps a vertex attribute to an accessor, an accessor contains data
    // telling us how to read/access the data to make sense of it.
    const cgltf_accessor *accessor =
        mesh->primitives->attributes[foundIdx].data;
    // the buffer view will tell us which region of the buffer contains the data
    // we need
    const cgltf_buffer_view *view = accessor->buffer_view;

    // making sure all the vertex attributes have
    int accessorCount = static_cast<int>(accessor->count);
    uniqueVerticesCount =
        uniqueVerticesCount == -1 ? accessorCount : uniqueVerticesCount;
    if (uniqueVerticesCount != accessorCount) {
      printf("[ERROR] Mismatched mesh attribute count for index %i. Required "
             "%i but got %i",
             attrIdx, uniqueVerticesCount, accessorCount);
      return {};
    }

    const float *dataToCopy =
        reinterpret_cast<float *>((char *)view->buffer->data + view->offset);

    // this tell us the component size in bytes, so if we have a vec3, it means
    // we have 3 components, each component will have size 4bytes
    int datatypeSize = component_size(accessor->component_type);
    if (datatypeSize != 4) {
      printf("Vertex attribute with index %i has unexpected component width. "
             "Expected 4, got %i",
             attrIdx, datatypeSize);
      return {};
    }
    int componentInType = accessor->type;

    // we are normalizing the data to float4 or float2, this means we will need
    // to fill a value if we get a vec3, we use a different filler per attribute
    float filler = MESH_ATTRIBUTES_COMPONENT_FILLER[attrIdx];

    auto size = static_cast<uint32_t>(meshData.size());
    // if we have an empty vector we don't need any alignment, the first
    // alignment will be taken care by the driver
    if (size != 0) {
      uint32_t mod = size % aligmentInFloats;
      uint32_t toAdd = aligmentInFloats - mod;
      aligmentOffsets += toAdd;
      for (uint32_t p = 0; p < toAdd; ++p) {
        meshData.push_back(0.0f);
      }
    }
    assert((meshData.size() % aligmentInFloats) == 0);
    // we keep track of how many elements we had before adding the new attribute
    // and how many we added in the end, we will use this data to compute the
    // offset and view size
    size_t start = meshData.size();

    for (int idx = 0; idx < uniqueVerticesCount; ++idx) {
      meshData.push_back(dataToCopy[idx * componentInType + 0]);
      meshData.push_back(dataToCopy[idx * componentInType + 1]);
      if (attrIdx != MESH_ATTRIBUTE_TYPE_UV) {
        // if is not a UV (float2) we push in the 3rd parameter plus filler
        meshData.push_back(dataToCopy[idx * componentInType + 2]);
        meshData.push_back(filler);
      }
    }

    size_t end = static_cast<uint32_t>(meshData.size());

    if (attrIdx == MESH_ATTRIBUTE_TYPE_POSITION) {
      // generate bounding boxes for the positions
      float minX = std::numeric_limits<float>::max();
      float minY = minX;
      float minZ = minY;
      float maxX = std::numeric_limits<float>::min();
      float maxY = maxX;
      float maxZ = maxX;
      const float *posData = meshData.data();
      for (size_t p = start; p < end; p += 4) {
        const float *vtx = posData + p;

        // lets us compute bounding box
        minX = vtx[0] < minX ? vtx[0] : minX;
        minY = vtx[1] < minY ? vtx[1] : minY;
        minZ = vtx[2] < minZ ? vtx[2] : minZ;

        maxX = vtx[0] > maxX ? vtx[0] : maxX;
        maxY = vtx[1] > maxY ? vtx[1] : maxY;
        maxZ = vtx[2] > maxZ ? vtx[2] : maxZ;
      }
      outMesh.m_boundingBox[0] = minX;
      outMesh.m_boundingBox[1] = minY;
      outMesh.m_boundingBox[2] = minZ;
      outMesh.m_boundingBox[3] = maxX;
      outMesh.m_boundingBox[4] = maxY;
      outMesh.m_boundingBox[5] = maxZ;
    }

    outMesh.ranges[attrIdx] = {
        static_cast<uint32_t>(start * sizeof(float)),
        static_cast<uint32_t>((end - start) * sizeof(float))};
  }

  // this just validates that we processed the same amount of data we expected
  assert(static_cast<int>(meshData.size()) ==
         ((uniqueVerticesCount * 14) + aligmentOffsets));

  // processing the index buffer
  const cgltf_accessor *accessor = prim.indices;
  const cgltf_buffer_view *view = accessor->buffer_view;
  cgltf_size componentSize = component_size(accessor->component_type);

  outMesh.primitivesCount = static_cast<uint32_t>(accessor->count);
  std::vector<uint32_t> indices;
  indices.reserve(outMesh.primitivesCount);
  if (componentSize == 4) {
    const auto *source = reinterpret_cast<uint32_t const *>(
        (char *)view->buffer->data + view->offset);
    for (size_t idx = 0; idx < accessor->count; ++idx) {
      indices.push_back(source[idx]);
    }
  } else {
    if (componentSize != 2) {
      printf("Mesh index buffer needs to be either 4 or 2 bytes, got %i",
             componentSize);
      return {};
    }
    const auto *source = reinterpret_cast<uint16_t const *>(
        (char *)view->buffer->data + view->offset);
    for (size_t idx = 0; idx < accessor->count; ++idx) {
      indices.push_back(source[idx]);
    }
  }

  BufferHandle vhandle =
      m_allocator.allocate(sizeof(float) * meshData.size(),
                           (std::string(mesh->name) + "Vertices").c_str(),
                           BUFFER_FLAG_GPU_ONLY, meshData.data());
  id vertexBuffer = m_allocator.getBuffer(vhandle);

  BufferHandle ihandle =
      m_allocator.allocate(indices.size() * sizeof(uint32_t),
                           (std::string(mesh->name) + "Indices").c_str(),
                           BUFFER_FLAG_GPU_ONLY, indices.data());
  id indexBuffer = m_allocator.getBuffer(ihandle);

  outMesh.name = mesh->name;
  outMesh.indexBuffer = indexBuffer;
  outMesh.vertexBuffer = vertexBuffer;
  outMesh.m_indexHandle = ihandle;
  outMesh.m_vertexHandle = vhandle;

  uint32_t index = m_meshCounter++;
  m_handleToMesh[index] = outMesh;
  // NOTE we are not adding the handle to the look up by name because this comes
  // from a gltf file, meaning multiple meshes in a file
  auto handle = getHandle<MeshHandle>(index);
  return handle;
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

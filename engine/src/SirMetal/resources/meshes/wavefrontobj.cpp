
#include "wavefrontobj.h"
#include "meshoptimizer.h"

#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"
#include "objparser.h"

uint64_t alignSize(const uint64_t sizeInBytes, const uint64_t boundaryInByte,
                   uint64_t &offset) {
  uint64_t modulus = sizeInBytes % boundaryInByte;
  offset = boundaryInByte - modulus;
  return sizeInBytes + offset;
}
bool loadMeshObj(MeshObj &result, const char *path) {

  ObjFile file;
  if (!objParseFile(file, path))
    return false;

  size_t index_count = file.f_size / 3;

  std::vector<Vertex> vertices(index_count);
  std::vector<float> positions(index_count * 4);
  std::vector<float> normals(index_count * 4);
  std::vector<float> uvs(index_count * 4);
  std::vector<float> tangents(index_count * 4);

  for (int i = 0; i < index_count * 4; ++i) {
    tangents[i] = 0.0f;
    normals[i] = 0.0f;
    uvs[i] = 0.0f;
  }

  for (size_t i = 0; i < index_count; ++i) {

    int vi = file.f[i * 3 + 0];
    int vti = file.f[i * 3 + 1];
    int vni = file.f[i * 3 + 2];

    // Vertex &v = vertices[i];
    // v.vx = file.v[vi * 3 + 0];
    // v.vy = file.v[vi * 3 + 1];
    // v.vz = file.v[vi * 3 + 2];
    // v.vw = 1.0f;
    // v.nx = vni < 0 ? 0.f : file.vn[vni * 3 + 0];
    // v.ny = vni < 0 ? 0.f : file.vn[vni * 3 + 1];
    // v.nz = vni < 0 ? 1.f : file.vn[vni * 3 + 2];
    // v.tu = vti < 0 ? 0.f : file.vt[vti * 3 + 0];
    // v.tv = vti < 0 ? 0.f : file.vt[vti * 3 + 1];

    assert((i * 4 + 3) < (index_count * 4));
    positions[i * 4 + 0] = file.v[vi * 3 + 0];
    positions[i * 4 + 1] = file.v[vi * 3 + 1];
    positions[i * 4 + 2] = file.v[vi * 3 + 2];
    positions[i * 4 + 3] = 1.0f;

    if (vni >= 0) {
      assert((i * 4 + 3) < (index_count * 4));
      normals[i * 4 + 0] = file.vn[vni * 3 + 0];
      normals[i * 4 + 1] = file.vn[vni * 3 + 1];
      normals[i * 4 + 2] = file.vn[vni * 3 + 2];
      normals[i * 4 + 3] = 0.0f;
    }

    if (vti >= 0) {
      assert((vti * 4 + 3) < (index_count * 4));
      uvs[i * 4 + 0] = file.vt[vti * 3 + 0];
      uvs[i * 4 + 1] = file.vt[vti * 3 + 1];
      uvs[i * 4 + 2] = 0.0f;
      uvs[i * 4 + 3] = 0.0f;
    }
  }

  meshopt_Stream streams[] = {
      {&positions[0], sizeof(float) * 4, sizeof(float) * 4},
      {&normals[0], sizeof(float) * 4, sizeof(float) * 4},
      {&uvs[0], sizeof(float) * 4, sizeof(float) * 4},
      {&tangents[0], sizeof(float) * 4, sizeof(float) * 4},
  };

  std::vector<uint32_t> remap(index_count);
  size_t vertex_count = meshopt_generateVertexRemapMulti(
      remap.data(), nullptr, index_count, index_count, streams, 4);

  std::vector<float> posOut(vertex_count * 4);
  std::vector<float> nOut(vertex_count * 4);
  std::vector<float> uvOut(vertex_count * 4);
  std::vector<float> tOut(vertex_count * 4);

  result.vertices.resize(vertex_count);
  result.indices.resize(index_count);

  meshopt_remapVertexBuffer(posOut.data(), positions.data(), index_count,
                            sizeof(float) * 4, remap.data());
  meshopt_remapVertexBuffer(nOut.data(), normals.data(), index_count,
                            sizeof(float) * 4, remap.data());
  meshopt_remapVertexBuffer(uvOut.data(), uvs.data(), index_count,
                            sizeof(float) * 4, remap.data());
  meshopt_remapVertexBuffer(tOut.data(), tangents.data(), index_count,
                            sizeof(float) * 4, remap.data());

  meshopt_remapIndexBuffer(result.indices.data(), 0, index_count, remap.data());

  meshopt_optimizeVertexCache(result.indices.data(), result.indices.data(),
                              index_count, vertex_count);
  // now I need to merge the data and generate the memory ranges
  // lets compute all the offsets
  uint32_t alignRequirement = sizeof(float) * 64;
  uint64_t pointSizeInByte = posOut.size() * sizeof(float) * 4;
  uint64_t normalPointerOffset = 0;
  uint64_t normalsOffsetByte =
      alignSize(pointSizeInByte, alignRequirement, normalPointerOffset);

  uint64_t normalsSize = nOut.size() * sizeof(float) * 4;
  uint64_t uvPointerOffset = 0;
  uint64_t uvOffsetByte = alignSize(normalsOffsetByte + normalsSize,
                                    alignRequirement, uvPointerOffset);

  uint64_t uvSize = uvOut.size() * sizeof(float) * 4;
  uint64_t tangentsPointerOffset = 0;
  uint64_t tangentsOffsetByte =
      alignSize(uvOffsetByte + uvSize, alignRequirement, tangentsPointerOffset);

  uint64_t totalRequiredAligmentBytes =
      normalPointerOffset + uvPointerOffset + tangentsPointerOffset;
  uint64_t totalRequiredAligmentFloats = totalRequiredAligmentBytes / 4;

  // pos
  result.ranges[0].m_offset = 0;
  result.ranges[0].m_size = static_cast<uint32_t>(pointSizeInByte);
  // normals
  result.ranges[1].m_offset = static_cast<uint32_t>(normalsOffsetByte);
  result.ranges[1].m_size = static_cast<uint32_t>(normalsSize);
  // uvs
  result.ranges[2].m_offset = static_cast<uint32_t>(uvOffsetByte);
  result.ranges[2].m_size = static_cast<uint32_t>(uvSize);
  // tans
  result.ranges[3].m_offset = static_cast<uint32_t>(tangentsOffsetByte);
  result.ranges[3].m_size =
      static_cast<uint32_t>(tOut.size() * sizeof(float) * 4);

  // lets do the memcopies
  // positions : vec4
  // normals : vec4
  // uvs : vec4
  // tangents vec4
  uint64_t floatsPerVertex = 4 + 4 + 4 + 4;
  uint64_t totalRequiredMemoryInFloats =
      vertex_count * floatsPerVertex + totalRequiredAligmentFloats;

  result.vertices.resize(totalRequiredMemoryInFloats);
  memcpy((char *)result.vertices.data(), posOut.data(),
         result.ranges[0].m_size);
  memcpy(((char *)result.vertices.data()) + result.ranges[1].m_offset,
         nOut.data(), result.ranges[1].m_size);
  memcpy(((char *)result.vertices.data()) + result.ranges[2].m_offset,
         uvOut.data(), result.ranges[2].m_size);
  memcpy(((char *)result.vertices.data()) + result.ranges[3].m_offset,
         uvOut.data(), result.ranges[3].m_size);
  assert((((char *)result.vertices.data()) + result.ranges[3].m_offset +
          result.ranges[3].m_size) <
         (char *)(result.vertices.data() + result.vertices.size()));

  // TODO: optimize the mesh for more efficient GPU rendering
  return true;
}

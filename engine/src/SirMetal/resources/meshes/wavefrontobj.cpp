
#include "SirMetal/resources/meshes/wavefrontobj.h"
#include "SirMetal/resources/meshes/objparser.h"

#define FAST_OBJ_IMPLEMENTATION
#include "SirMetal/resources/meshes/fast_obj.h"

#include "meshoptimizer.h"

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

  // not super happy about this but for now it will do, ideally you should have
  // a pool of scratch memory that can be used
  std::vector<float> positions(index_count * 4);
  std::vector<float> normals(index_count * 4);
  std::vector<float> uvs(index_count * 2);
  std::vector<float> tangents(index_count * 4);

  for (int i = 0; i < index_count * 2; ++i) {
    uvs[i] = 0.0f;
  }
  for (int i = 0; i < index_count * 4; ++i) {
    tangents[i] = 0.0f;
    normals[i] = 0.0f;
  }

  // let us extract the data from the obj, for later manipulation
  //#pragma omp parallel for
  for (size_t i = 0; i < index_count; ++i) {

    int vi = file.f[i * 3 + 0];
    int vti = file.f[i * 3 + 1];
    int vni = file.f[i * 3 + 2];

    assert((i * 4 + 3) < (index_count * 4));
    positions[i * 4 + 0] = file.v[vi * 3 + 0];
    positions[i * 4 + 1] = file.v[vi * 3 + 1];
    positions[i * 4 + 2] = file.v[vi * 3 + 2];
    positions[i * 4 + 3] = 1.0f;

    if ((vni >= 0) & (file.vn != nullptr)) {
      assert((i * 4 + 3) < (index_count * 4));
      normals[i * 4 + 0] = file.vn[vni * 3 + 0];
      normals[i * 4 + 1] = file.vn[vni * 3 + 1];
      normals[i * 4 + 2] = file.vn[vni * 3 + 2];
      normals[i * 4 + 3] = 0.0f;
    }

    if ((vti >= 0) & (file.vt != nullptr)) {
      assert((i * 2 + 1) < (index_count * 2));
      uvs[i * 2 + 0] = file.vt[vti * 3 + 0];
      uvs[i * 2 + 1] = file.vt[vti * 3 + 1];
    }
  }

  // mesh optimizer pass for doing both an index buffer and some optimizations
  // since we want de-interleaved data we need to use different streams
  // TODO we might be able to not re-copy the data from the obj ,remap it with
  // the correct stride
  meshopt_Stream streams[] = {
      {&positions[0], sizeof(float) * 4, sizeof(float) * 4},
      {&normals[0], sizeof(float) * 4, sizeof(float) * 4},
      {&uvs[0], sizeof(float) * 2, sizeof(float) * 2},
      {&tangents[0], sizeof(float) * 4, sizeof(float) * 4},
  };

  // building the remapper data structure
  std::vector<uint32_t> remap(index_count);
  size_t vertex_count = meshopt_generateVertexRemapMulti(
      remap.data(), nullptr, index_count, index_count, streams, 4);

  // allocating necessary memory for storing remapper result
  std::vector<float> posOut(vertex_count * 4);
  std::vector<float> nOut(vertex_count * 4);
  std::vector<float> uvOut(vertex_count * 2);
  std::vector<float> tOut(vertex_count * 4);
  result.indices.resize(index_count);

  //remap the vertices, one stream at the time
  meshopt_remapVertexBuffer(posOut.data(), positions.data(), index_count,
                            sizeof(float) * 4, remap.data());
  meshopt_remapVertexBuffer(nOut.data(), normals.data(), index_count,
                            sizeof(float) * 4, remap.data());
  meshopt_remapVertexBuffer(uvOut.data(), uvs.data(), index_count,
                            sizeof(float) * 2, remap.data());
  meshopt_remapVertexBuffer(tOut.data(), tangents.data(), index_count,
                            sizeof(float) * 4, remap.data());

  std::vector<uint32_t> tmp(index_count);
  //remapping index buffer and optimize for vertex cache reuse
  meshopt_remapIndexBuffer(tmp.data(), nullptr, index_count,
                           remap.data());

  meshopt_optimizeVertexCache(result.indices.data(), tmp.data(),
                              index_count, vertex_count);

  // now I need to merge the data and generate the memory ranges
  // lets compute all the alignement offsets
  constexpr uint32_t alignRequirement = 256; // in bytes
  uint64_t pointSizeInByte = posOut.size() * sizeof(float);
  uint64_t normalPointerOffset = 0;
  uint64_t normalsOffsetByte =
      alignSize(pointSizeInByte, alignRequirement, normalPointerOffset);

  uint64_t normalsSize = nOut.size() * sizeof(float);
  uint64_t uvPointerOffset = 0;
  uint64_t uvOffsetByte = alignSize(normalsOffsetByte + normalsSize,
                                    alignRequirement, uvPointerOffset);

  uint64_t uvSize = uvOut.size() * sizeof(float);
  uint64_t tangentsPointerOffset = 0;
  uint64_t tangentsOffsetByte =
      alignSize(uvOffsetByte + uvSize, alignRequirement, tangentsPointerOffset);

  uint64_t totalRequiredAlignmentBytes =
      normalPointerOffset + uvPointerOffset + tangentsPointerOffset;
  uint64_t totalRequiredAlignmentFloats = totalRequiredAlignmentBytes / 4;

  // generating the memory ranges
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
  result.ranges[3].m_size = static_cast<uint32_t>(tOut.size() * sizeof(float));

  // lets do the memcopies
  // positions : vec4
  // normals : vec4
  // uvs : vec2
  // tangents vec4
  uint64_t floatsPerVertex = 4 + 4 + 2 + 4;
  uint64_t totalRequiredMemoryInFloats =
      (vertex_count * floatsPerVertex) + totalRequiredAlignmentFloats;

  result.vertices.resize(totalRequiredMemoryInFloats);
  memcpy((char *)result.vertices.data(), posOut.data(),
         result.ranges[0].m_size);
  memcpy(((char *)result.vertices.data()) + result.ranges[1].m_offset,
         nOut.data(), result.ranges[1].m_size);
  memcpy(((char *)result.vertices.data()) + result.ranges[2].m_offset,
         uvOut.data(), result.ranges[2].m_size);
  memcpy(((char *)result.vertices.data()) + result.ranges[3].m_offset,
         tOut.data(), result.ranges[3].m_size);
  [[maybe_unused]] auto totalComputedBuffer =
      (((char *)result.vertices.data()) + result.ranges[3].m_offset +
       result.ranges[3].m_size);
  [[maybe_unused]] auto arraySize =
      (char *)(result.vertices.data()) + (result.vertices.size() * 4);

  assert(totalComputedBuffer == arraySize);

  return true;
}

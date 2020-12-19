#include "SirMetal/resources/meshes/meshOptimize.h"
#include "meshoptimizer.h"

namespace SirMetal {
uint64_t alignSize(const uint64_t sizeInBytes, const uint64_t boundaryInByte,
                   uint64_t &offset) {
  uint64_t modulus = sizeInBytes % boundaryInByte;
  offset = boundaryInByte - modulus;
  return sizeInBytes + offset;
}

void optimizeRawDeinterleavedMesh(const MapperData &data) {
  // mesh optimizer pass for doing both an index buffer and some optimizations
  // since we want de-interleaved data we need to use different streams
  // TODO we might be able to not re-copy the data from the obj ,remap it with
  // the correct stride
  meshopt_Stream streams[] = {
      {data.pIn->data(), sizeof(float) * 4, sizeof(float) * 4},
      {data.nIn->data(), sizeof(float) * 4, sizeof(float) * 4},
      {data.uvIn->data(), sizeof(float) * 2, sizeof(float) * 2},
      {data.tIn->data(), sizeof(float) * 4, sizeof(float) * 4},
  };

  // building the remapper data structure
  std::vector<uint32_t> remap(data.indexCount);
  size_t vertex_count = meshopt_generateVertexRemapMulti(
      remap.data(), nullptr, data.indexCount, data.indexCount, streams, 4);

  // allocating necessary memory for storing remapper result
  data.pOut->resize(vertex_count*4);
  data.nOut->resize(vertex_count*4);
  data.uvOut->resize(vertex_count*2);
  data.tOut->resize(vertex_count*4);
  data.outIndex->resize(data.indexCount);

  //remap the vertices, one stream at the time
  meshopt_remapVertexBuffer(data.pOut->data(), data.pIn->data(), data.indexCount,
                            sizeof(float) * 4, remap.data());
  meshopt_remapVertexBuffer(data.nOut->data(), data.nIn->data(), data.indexCount,
                            sizeof(float) * 4, remap.data());
  meshopt_remapVertexBuffer(data.uvOut->data(), data.uvIn->data(), data.indexCount,
                            sizeof(float) * 2, remap.data());
  meshopt_remapVertexBuffer(data.tOut->data(), data.tIn->data(), data.indexCount,
                            sizeof(float) * 4, remap.data());

  std::vector<uint32_t> tmp(data.indexCount);
  //remapping index buffer and optimize for vertex cache reuse
  meshopt_remapIndexBuffer(tmp.data(), nullptr, data.indexCount,
                           remap.data());

  meshopt_optimizeVertexCache(data.outIndex->data(), tmp.data(),
                              data.indexCount, vertex_count);


}
void mergeRawMeshBuffers(
    const std::vector<float>& pos,
    const std::vector<float>& normals,
    const std::vector<float>& uvs,
    const std::vector<float>& tans,
    std::vector<float>& outData, MemoryRange *ranges) {
  // now I need to merge the data and generate the memory ranges
  // lets compute all the alignment offsets
  constexpr uint32_t alignRequirement = 256; // in bytes
  uint64_t pointSizeInByte = pos.size() * sizeof(float);
  uint64_t normalPointerOffset = 0;
  uint64_t normalsOffsetByte =
      alignSize(pointSizeInByte, alignRequirement, normalPointerOffset);

  uint64_t normalsSize = normals.size() * sizeof(float);
  uint64_t uvPointerOffset = 0;
  uint64_t uvOffsetByte = alignSize(normalsOffsetByte + normalsSize,
                                    alignRequirement, uvPointerOffset);

  uint64_t uvSize = uvs.size() * sizeof(float);
  uint64_t tangentsPointerOffset = 0;
  uint64_t tangentsOffsetByte =
      alignSize(uvOffsetByte + uvSize, alignRequirement, tangentsPointerOffset);

  uint64_t totalRequiredAlignmentBytes =
      normalPointerOffset + uvPointerOffset + tangentsPointerOffset;
  uint64_t totalRequiredAlignmentFloats = totalRequiredAlignmentBytes / 4;

  // generating the memory ranges
  // pos
  ranges[0].m_offset = 0;
  ranges[0].m_size = static_cast<uint32_t>(pointSizeInByte);
  // normals
  ranges[1].m_offset = static_cast<uint32_t>(normalsOffsetByte);
  ranges[1].m_size = static_cast<uint32_t>(normalsSize);
  // uvs
  ranges[2].m_offset = static_cast<uint32_t>(uvOffsetByte);
  ranges[2].m_size = static_cast<uint32_t>(uvSize);
  // tans
  ranges[3].m_offset = static_cast<uint32_t>(tangentsOffsetByte);
  ranges[3].m_size = static_cast<uint32_t>(tans.size() * sizeof(float));

  // lets do the memcopies
  // positions : vec4
  // normals : vec4
  // uvs : vec2
  // tangents vec4
  uint32_t vertex_count = pos.size()/4;
  uint64_t floatsPerVertex = 4 + 4 + 2 + 4;
  uint64_t totalRequiredMemoryInFloats =
      (vertex_count * floatsPerVertex) + totalRequiredAlignmentFloats;

  outData.resize(totalRequiredMemoryInFloats);
  memcpy((char *)outData.data(), pos.data(),
         ranges[0].m_size);
  memcpy(((char *)outData.data()) + ranges[1].m_offset,
         normals.data(), ranges[1].m_size);
  memcpy(((char *)outData.data()) + ranges[2].m_offset,
         uvs.data(), ranges[2].m_size);
  memcpy(((char *)outData.data()) + ranges[3].m_offset,
         tans.data(), ranges[3].m_size);
  [[maybe_unused]] auto totalComputedBuffer =
      (((char *)outData.data()) + ranges[3].m_offset +
       ranges[3].m_size);
  [[maybe_unused]] auto arraySize =
      (char *)(outData.data()) + (outData.size() * 4);
  assert(totalComputedBuffer == arraySize);


}
} // namespace SirMetal
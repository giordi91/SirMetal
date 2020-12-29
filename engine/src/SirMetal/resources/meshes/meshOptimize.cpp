#include "SirMetal/resources/meshes/meshOptimize.h"
#include "meshoptimizer.h"

namespace SirMetal {
uint64_t alignSize(const uint64_t sizeInBytes, const uint64_t boundaryInByte,
                   uint64_t &offset) {
  uint64_t modulus = sizeInBytes % boundaryInByte;
  offset = modulus != 0 ? boundaryInByte - modulus : 0;
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
  data.pOut->resize(vertex_count * 4);
  data.nOut->resize(vertex_count * 4);
  data.uvOut->resize(vertex_count * 2);
  data.tOut->resize(vertex_count * 4);
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
  meshopt_remapIndexBuffer(tmp.data(), nullptr, data.indexCount, remap.data());

  meshopt_optimizeVertexCache(data.outIndex->data(), tmp.data(), data.indexCount,
                              vertex_count);
}
void mergeRawMeshBuffers(const std::vector<float> *attributes, float *strides,
                         uint32_t count, std::vector<float> &outData,
                         MemoryRange *ranges) {


  // now I need to merge the data and generate the memory ranges
  // lets compute all the alignment offsets
  constexpr uint32_t alignRequirement = 256;// in bytes
  uint64_t totalRequiredAlignmentFloats = 0;
  uint64_t prevSize = 0;
  uint64_t offsetByte = 0;
  uint64_t floatsPerVertex = 0;

  //we iterate all the attributes, accumulating required padding  for aligment
  //and recording actual final memory ranges
  for (int i = 0; i < count; ++i) {
    const std::vector<float> &attribute = attributes[i];
    uint64_t currSize = attribute.size() * sizeof(float);
    uint64_t pointerOffset = 0;

    //this function computes the aligment, it returns the aligment offset and also how much
    //we had to align for.
    offsetByte = alignSize(prevSize + offsetByte, alignRequirement, pointerOffset);
    //we accumulate all the required alignment such that later on we can compute the required total memory
    totalRequiredAlignmentFloats += pointerOffset;


    //recording the memory range for this attribute
    ranges[i].m_offset = offsetByte;
    ranges[i].m_size = currSize;

    prevSize =  currSize;
    floatsPerVertex += strides[i];
  }
  //we need the size in floats since the output buffer is a float buffer
  totalRequiredAlignmentFloats/=4;

  // lets do the mem-copies
  uint32_t vertexCount = attributes[0].size() / 4;
  //allocating enough memory
  uint64_t totalRequiredMemoryInFloats =
          (vertexCount * floatsPerVertex) + totalRequiredAlignmentFloats;
  outData.resize(totalRequiredMemoryInFloats);

  //here we compare that what we got from the memory ranges actually matches what we exect
  //in the allocation size
  [[maybe_unused]] auto totalComputedBuffer =
          (((char *) outData.data()) + ranges[count - 1].m_offset +
           ranges[count - 1].m_size);
  [[maybe_unused]] auto arraySize = (char *) (outData.data()) + (outData.size() * 4);
  assert(totalComputedBuffer == arraySize);

  //perform the copies
  for (int i = 0; i < count; ++i) {
    memcpy(((char *) outData.data()) + ranges[i].m_offset, attributes[i].data(),
           ranges[i].m_size);
  }
}
void optimizeVertexCache(std::vector<uint32_t> &outIndices,
                         const std::vector<uint32_t> &inIndices, uint32_t indexCount,
                         uint32_t vertexCount) {
  meshopt_optimizeVertexCache(outIndices.data(), inIndices.data(), indexCount,
                              vertexCount);
}
}// namespace SirMetal
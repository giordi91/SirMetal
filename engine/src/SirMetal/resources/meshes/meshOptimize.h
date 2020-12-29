#pragma  once

#include <vector>
#include <stdint.h>

#include "SirMetal/core/core.h"
namespace SirMetal
{
struct MapperData
{
  const std::vector<float>* pIn;
  const std::vector<float>* nIn;
  const std::vector<float>* uvIn;
  const std::vector<float>* tIn;
  std::vector<float>* pOut;
  std::vector<float>* nOut;
  std::vector<float>* uvOut;
  std::vector<float>* tOut;
  std::vector<uint32_t>* outIndex;
  uint32_t indexCount;
};

void optimizeRawDeinterleavedMesh( const MapperData& data);
void optimizeVertexCache(std::vector<uint32_t> &outIndices, const std::vector<uint32_t> &inIndices,
                         uint32_t indexCount, uint32_t vertexCount);

void mergeRawMeshBuffers(
        const std::vector<float> *attributes, float *strides,
        uint32_t count, std::vector<float> &outData,
        MemoryRange *ranges);


}
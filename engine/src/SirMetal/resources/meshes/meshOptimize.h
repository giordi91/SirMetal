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
void mergeRawMeshBuffers(
    const std::vector<float>& pos,
    const std::vector<float>& normals,
    const std::vector<float>& uvs,
    const std::vector<float>& tans,
    std::vector<float>& outData, MemoryRange *ranges);


}
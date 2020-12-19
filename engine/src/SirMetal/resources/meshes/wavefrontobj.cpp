
#include "SirMetal/resources/meshes/wavefrontobj.h"
#include "SirMetal/resources/meshes/meshOptimize.h"
#include "SirMetal/resources/meshes/objparser.h"

#include <math.h>

#define FAST_OBJ_IMPLEMENTATION
#include "SirMetal/resources/meshes/fast_obj.h"

#include "meshoptimizer.h"

namespace SirMetal {
bool loadMeshObj(MeshLoadResult &result, const char *path) {

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

  std::vector<float> posOut;
  std::vector<float> nOut;
  std::vector<float> uvOut;
  std::vector<float> tOut;
  result.indices.resize(index_count);
  SirMetal::MapperData mapper{&positions,
                              &normals,
                              &uvs,
                              &tangents,
                              &posOut,
                              &nOut,
                              &uvOut,
                              &tOut,
                              &result.indices,
                              static_cast<uint32_t>(index_count)};

  // generate an index buffer and optimize per vertex cache hit using mesh
  // optimizer
  SirMetal::optimizeRawDeinterleavedMesh(mapper);

  // merge the buffer into a single one
  SirMetal::mergeRawMeshBuffers(posOut, nOut, uvOut, tOut, result.vertices,
                                result.ranges);

  return true;
}

} // namespace SirMetal
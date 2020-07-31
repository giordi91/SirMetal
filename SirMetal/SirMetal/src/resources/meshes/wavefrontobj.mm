
#include "wavefrontobj.h"
#include "meshoptimizer.h"

#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"
#include "objparser.h"


bool loadMesh(Mesh& result, const char* path)
{

    ObjFile file;
    if (!objParseFile(file, path))
        return false;

    size_t index_count = file.f_size / 3;

    std::vector<Vertex> vertices(index_count);

    for (size_t i = 0; i < index_count; ++i)
    {
        Vertex& v = vertices[i];

        int vi = file.f[i * 3 + 0];
        int vti = file.f[i * 3 + 1];
        int vni = file.f[i * 3 + 2];

        v.vx = file.v[vi * 3 + 0];
        v.vy = file.v[vi * 3 + 1];
        v.vz = file.v[vi * 3 + 2];
        v.nx = vni < 0 ? 0.f : file.vn[vni * 3 + 0];
        v.ny = vni < 0 ? 0.f : file.vn[vni * 3 + 1];
        v.nz = vni < 0 ? 1.f : file.vn[vni * 3 + 2];
        v.tu = vti < 0 ? 0.f : file.vt[vti * 3 + 0];
        v.tv = vti < 0 ? 0.f : file.vt[vti * 3 + 1];
    }

    std::vector<uint32_t> remap(index_count);
    size_t vertex_count = meshopt_generateVertexRemap(remap.data(), 0, index_count, vertices.data(), index_count, sizeof(Vertex));

    result.vertices.resize(vertex_count);
    result.indices.resize(index_count);

    meshopt_remapVertexBuffer(result.vertices.data(), vertices.data(), index_count, sizeof(Vertex), remap.data());
    meshopt_remapIndexBuffer(result.indices.data(), 0, index_count, remap.data());

    // TODO: optimize the mesh for more efficient GPU rendering
    return true;
}


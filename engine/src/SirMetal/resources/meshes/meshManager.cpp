
#include "SirMetal/resources/meshes/meshManager.h"
#import "SirMetal/resources/meshes/meshOptimize.h"
#import "SirMetal/resources/meshes/wavefrontobj.h"
#import "SirMetal/resources/meshes/gltfMesh.h"
#include <SirMetal/io/file.h>

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

    MeshHandle MeshManager::processObjMesh(const std::string &path) {

        assert(fileExists(path));
        MeshLoadResult result;
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

    MeshHandle MeshManager::loadFromMemory(const void *data, LOAD_MESH_TYPE type) {

        MeshLoadResult result;
        switch (type) {

            case LOAD_MESH_TYPE::INVALID: {
                assert(0 && "mesh type is invalid");
                break;
            }
            case LOAD_MESH_TYPE::GLTF_MESH: {
                loadGltfMesh(result, data);
            }
        }


        BufferHandle vhandle =
                m_allocator.allocate(sizeof(float) * result.vertices.size(),
                                     (result.name + "Vertices").c_str(),
                                     BUFFER_FLAG_GPU_ONLY, result.vertices.data());
        id vertexBuffer = m_allocator.getBuffer(vhandle);

        BufferHandle ihandle =
                m_allocator.allocate(result.indices.size() * sizeof(uint32_t),
                                     (result.name + "Indices").c_str(),
                                     BUFFER_FLAG_GPU_ONLY, result.indices.data());
        id indexBuffer = m_allocator.getBuffer(ihandle);

        MeshData outMesh{};
        outMesh.name = result.name;
        outMesh.indexBuffer = indexBuffer;
        outMesh.vertexBuffer = vertexBuffer;
        outMesh.m_indexHandle = ihandle;
        outMesh.m_vertexHandle = vhandle;
        outMesh.primitivesCount = result.indices.size();
        outMesh.ranges[0] = result.ranges[0];
        outMesh.ranges[1] = result.ranges[1];
        outMesh.ranges[2] = result.ranges[2];
        outMesh.ranges[3] = result.ranges[3];

        uint32_t index = m_meshCounter++;
        m_handleToMesh[index] = outMesh;
        // NOTE we are not adding the handle to the look up by name because this comes
        // from a gltf file, meaning multiple meshes in a file
        auto handle = getHandle<MeshHandle>(index);
        return handle;
    }
} // namespace SirMetal

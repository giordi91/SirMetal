
#include "SirMetal/resources/meshes/meshManager.h"
#import "SirMetal/resources/meshes/wavefrontobj.h"
#import <Metal/Metal.h>
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
            }
        }

        return MeshHandle{};
    }

    id<MTLBuffer> createGPUOnlyBuffer(id<MTLDevice> currDevice,id<MTLCommandQueue> queue, void*data, size_t sizeInByte)
    {
        id<MTLBuffer> sharedBuffer = [currDevice newBufferWithBytes: data
                                                             length: sizeInByte
                                                            options:MTLResourceOptionCPUCacheModeDefault];

        id <MTLBuffer> _privateBuffer;
        _privateBuffer = [currDevice newBufferWithLength:sizeInByte
                                              options:MTLResourceStorageModePrivate];
        //need to perform the copy

        // Create a command buffer for GPU work.
        id <MTLCommandBuffer> commandBuffer = [queue commandBuffer];


// Encode a blit pass to copy data from the source buffer to the private buffer.
        id <MTLBlitCommandEncoder> blitCommandEncoder = [commandBuffer blitCommandEncoder];
        [blitCommandEncoder copyFromBuffer:sharedBuffer
                              sourceOffset:0
                                  toBuffer:_privateBuffer
                         destinationOffset:0 size:sizeInByte];
        [blitCommandEncoder endEncoding];


// Add a completion handler and commit the command buffer.
        [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> cb) {
            // Private buffer is populated.
        }];
        [commandBuffer commit];
        return _privateBuffer;
    }

    MeshHandle MeshManager::processObjMesh(const std::string &path) {

        MeshObj result;
        loadMeshObj(result, path.c_str());

        id<MTLDevice> currDevice = m_device;
        //TODO convert to a general buffer manager
        id<MTLBuffer> vertexBuffer = createGPUOnlyBuffer(currDevice, m_queue, result.vertices.data(),sizeof(Vertex)*result.vertices.size());
        [vertexBuffer setLabel:@"Vertices"];

        id<MTLBuffer> indexBuffer = createGPUOnlyBuffer(currDevice, m_queue, result.indices.data(),result.indices.size()*sizeof(uint32_t));
        [indexBuffer setLabel:@"Indices"];
        uint32_t index = m_meshCounter++;

        std::string meshName = getFileName(path);
        MeshData data {vertexBuffer,indexBuffer,static_cast<uint32_t>(result.indices.size()),std::move(meshName), matrix_float4x4_translation(vector_float3{0,0,0})};
        m_handleToMesh[index] = data;
        const std::string fileName = getFileName(path);
        MeshHandle handle = getHandle<MeshHandle>(index);
        m_nameToHandle[fileName] = handle.handle;
        return handle;
    }
    void MeshManager::cleanup() {}
    }

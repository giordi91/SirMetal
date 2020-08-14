
#include "meshManager.h"
#import "SirMetalLib/core/file.h"
#import "SirMetalLib/core/logging/log.h"
#import "wavefrontobj.h"
#import <Metal/Metal.h>

namespace SirMetal {
    MeshHandle MeshManager::loadMesh(const std::string &path) {

        const std::string extString = getFileExtension(path);
        FILE_EXT ext = getFileExtFromStr(extString);
        switch (ext) {
            case FILE_EXT::OBJ: {
                return processObjMesh(path);
            }
            default: {
                SIR_CORE_ERROR("Unsupported mesh extension {}", extString);
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
        //id<MTLBuffer> vertexBuffer = [currDevice newBufferWithBytes:result.vertices.data()
        //                                     length:sizeof(Vertex)*result.vertices.size()
        //                                    options:MTLResourceOptionCPUCacheModeDefault];
        id<MTLBuffer> vertexBuffer = createGPUOnlyBuffer(currDevice, m_queue, result.vertices.data(),sizeof(Vertex)*result.vertices.size());
        [vertexBuffer setLabel:@"Vertices"];

        //id<MTLBuffer> indexBuffer = [currDevice newBufferWithBytes:result.indices.data()
        //                                    length:result.indices.size()*sizeof(uint32_t)
        //                                   options:MTLResourceOptionCPUCacheModeDefault];
        id<MTLBuffer> indexBuffer = createGPUOnlyBuffer(currDevice, m_queue, result.indices.data(),result.indices.size()*sizeof(uint32_t));
        [indexBuffer setLabel:@"Indices"];
        uint32_t index = m_meshCounter++;

        MeshData data {vertexBuffer,indexBuffer,static_cast<uint32_t>(result.indices.size())};
        m_handleToMesh[index] = data;
        const std::string fileName = getFileName(path);
        MeshHandle handle = getHandle<MeshHandle>(index);
        m_nameToHandle[fileName] = handle.handle;
        return handle;
    }
}

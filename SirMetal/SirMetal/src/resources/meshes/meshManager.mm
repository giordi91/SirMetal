//
// Created by Marco Giordano on 31/07/2020.
// Copyright (c) 2020 Marco Giordano. All rights reserved.
//

#include "meshManager.h"
#import "file.h"
#import "log.h"
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

    MeshHandle MeshManager::processObjMesh(const std::string &path) {

        MeshObj result;
        loadMeshObj(result, path.c_str());

        id<MTLDevice> currDevice = m_device;
        id<MTLBuffer> vertexBuffer = [currDevice newBufferWithBytes:result.vertices.data()
                                             length:sizeof(Vertex)*result.vertices.size()
                                            options:MTLResourceOptionCPUCacheModeDefault];
        [vertexBuffer setLabel:@"Vertices"];

        id<MTLBuffer> indexBuffer = [currDevice newBufferWithBytes:result.indices.data()
                                            length:result.indices.size()*sizeof(uint32_t)
                                           options:MTLResourceOptionCPUCacheModeDefault];
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

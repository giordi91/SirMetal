#pragma once


#import <objc/objc.h>
#include "SirMetal/resources/handle.h"
#import <string>
#import <unordered_map>

#include "SirMetal/MBEMathUtilities.h"
namespace SirMetal {
    //TODO: temp public, we will need to build abstraction to render
    //this data potentially without the need to extract it from here
    //this is just an intermediate step
    struct MeshData {
        id vertexBuffer;
        id indexBuffer;
        uint32_t primitivesCount;
        std::string name;
        //Temp model matrix to allow manipulation
        matrix_float4x4 modelMatrix;
    };

    class MeshManager {
    public:
        MeshHandle loadMesh(const std::string &path);

        void initialize(id device, id queue) {
            m_device = device;
            m_queue = queue;
        };
        const MeshHandle getHandleFromName(const std::string& name) const
        {
            auto found = m_nameToHandle.find(name);
            if (found != m_nameToHandle.end()) {
                return {found->second};
            }
            return {};
        }

        const MeshData *getMeshData(MeshHandle handle) const {
            assert(getTypeFromHandle(handle) == HANDLE_TYPE::MESH);
            uint32_t index = getIndexFromHandle(handle);
            auto found = m_handleToMesh.find(index);
            if (found != m_handleToMesh.end()) {
                return &found->second;
            }
            return nullptr;
        }

    private:
        id m_device;
        id m_queue;
        std::unordered_map<uint32_t, MeshData> m_handleToMesh;
        std::unordered_map<std::string, uint32_t> m_nameToHandle;

        SirMetal::MeshHandle processObjMesh(const std::string &path);

        uint32_t m_meshCounter = 1;
    };

}

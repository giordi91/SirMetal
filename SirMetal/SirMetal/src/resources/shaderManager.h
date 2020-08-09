#pragma once

#import <objc/objc.h>

#include <unordered_map>
#include <string>
#include "handle.h"

namespace SirMetal {
    enum class SHADER_TYPE {
        RASTER,
        COMPUTE
    };

    class ShaderManager {
        struct ShaderMetadata {
            std::string libraryPath;
            SHADER_TYPE type;
            id vertexFn = nullptr;
            id fragFn = nullptr;
            id computeFn = nullptr;
            id library = nullptr;
        };
    public :
        LibraryHandle loadShader(const char *path);

        id getLibraryFromHandle(LibraryHandle handle);

        LibraryHandle getHandleFromName(const std::string &name) const;

        void initialize(id device, const std::string &resourcePath) {
            m_device = device;
            m_resourcePath = resourcePath;
        };

        id getVertexFunction(LibraryHandle handle);
        id getFragmentFunction(LibraryHandle handle);

    private:
        bool generateLibraryMetadata(id library,ShaderMetadata& metadata, const char* libraryPath);

    private:
        id m_device;
        std::string m_resourcePath;
        std::unordered_map<uint32_t, ShaderMetadata> m_libraries;
        std::unordered_map<std::string, uint32_t> m_nameToLibraryHandle;
        uint32_t m_libraryCounter = 1;
    };
}



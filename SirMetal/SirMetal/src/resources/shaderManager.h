#pragma once

#include <unordered_map>
#include <string>
#include "handle.h"

namespace SirMetal {
    class ShaderManager {
    public :
        LibraryHandle loadShader(const char *path);
        id getLibraryFromHandle(LibraryHandle handle);
        LibraryHandle getHandleFromName(const std::string& name) const;

        void initialize(id device, const std::string& resourcePath) {
            m_device = device;
            m_resourcePath = resourcePath;
        };

    private:
        id m_device;
        std::string m_resourcePath;
        std::unordered_map<uint32_t, id> m_libraries;
        std::unordered_map<std::string, uint32_t> m_nameToLibraryHandle;
        uint32_t m_libraryCounter = 0;
    };
}



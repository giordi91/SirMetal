#import <Metal/Metal.h>
#include "shaderManager.h"
#import "file.h"
#import "engineContext.h"

namespace SirMetal {
    LibraryHandle ShaderManager::loadShader(const char *path) {

        //checking if is already loaded
        const std::string fileName = getFileName(path);
        auto found = m_nameToLibraryHandle.find(fileName);
        if (found != m_nameToLibraryHandle.end()) {
            return LibraryHandle{found->second};
        }

        //loading the actual shader library
        NSString *shaderPath = [NSString stringWithCString:m_resourcePath.c_str()
                                                  encoding:[NSString defaultCStringEncoding]];
        __autoreleasing NSError *errorLib = nil;
        shaderPath = [shaderPath stringByAppendingString:@"/shaders/Shaders.metal"];

        NSString *content = [NSString stringWithContentsOfFile:shaderPath encoding:NSUTF8StringEncoding error:nil];
        id <MTLDevice> currDevice = m_device;
        id <MTLLibrary> libraryRaw = [currDevice newLibraryWithSource:content options:nil error:&errorLib];
        NSLog(@"%@", errorLib);
        uint32_t index = m_libraryCounter++;

        //updating the look ups
        m_libraries[index] = libraryRaw;
        m_nameToLibraryHandle[fileName] = index;

        return getHandle<LibraryHandle>(index);
    }

    id ShaderManager::getLibraryFromHandle(LibraryHandle handle) {
        uint32_t index = getIndexFromHandle(handle);
        return m_libraries[index];
    }

    LibraryHandle ShaderManager::getHandleFromName(const std::string &name) const {
        auto found = m_nameToLibraryHandle.find(name);
        if (found != m_nameToLibraryHandle.end()) {return {found->second};}
        return {};
    }


}


#import <Metal/Metal.h>
#include "shaderManager.h"
#import "engineContext.h"

namespace SirMetal {
    LibraryHandle ShaderManager::loadShader(const char *path, id <MTLDevice> device) {

        //checking if is already loaded
        auto found = m_nameToLibraryHandle.find(path);
        if(found != m_nameToLibraryHandle.end())
        {
            return LibraryHandle{found->second};
        }

        //loading the actual shader library
        NSString *shaderPath = [NSString stringWithCString:m_resourcePath.c_str()
                                                  encoding:[NSString defaultCStringEncoding]];
        __autoreleasing NSError *errorLib = nil;
        shaderPath = [shaderPath stringByAppendingString:@"/shaders/Shaders.metal"];

        NSString *content = [NSString stringWithContentsOfFile:shaderPath encoding:NSUTF8StringEncoding error:nil];
        id <MTLLibrary> libraryRaw = [device newLibraryWithSource:content options:nil error:&errorLib];
        uint32_t index = m_libraryCounter++;

        //updating the look ups
        m_libraries[index] = libraryRaw;
        m_nameToLibraryHandle[path] = index;

        return getHandle<LibraryHandle>(index);
    }

    id  ShaderManager::getLibraryFromHandle(LibraryHandle handle) {
        uint32_t index = getIndexFromHandle(handle);
        return m_libraries[index];
    }

}


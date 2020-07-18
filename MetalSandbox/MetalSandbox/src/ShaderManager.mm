//
// Created by Marco Giordano on 16/07/2020.
// Copyright (c) 2020 Marco Giordano. All rights reserved.
//

#import <Metal/Metal.h>
#include "ShaderManager.h"
#import "engineContext.h"

LibraryHandle ShaderManager::loadShader(const char *path, id <MTLDevice> device) {
    NSString *shaderPath = [NSString stringWithCString:SirMetal::CONTEXT->projectPath
                                              encoding:[NSString defaultCStringEncoding]];
    __autoreleasing NSError *errorLib = nil;
    shaderPath = [shaderPath stringByAppendingString:@"/shaders/Shaders.metal"];


    NSString *content = [NSString stringWithContentsOfFile:shaderPath encoding:NSUTF8StringEncoding error:nil];
    NSLog(shaderPath);
    id <MTLLibrary> libraryRaw = [device newLibraryWithSource:content options:nil error:&errorLib];
    m_shaders.push_back(libraryRaw);
    return m_shaders.size() - 1;
}

id<MTLLibrary> ShaderManager::getLibraryFromHandle(LibraryHandle handle) {
    assert(handle < m_shaders.size());
    return m_shaders[handle];
}

//
// Created by Marco Giordano on 16/07/2020.
// Copyright (c) 2020 Marco Giordano. All rights reserved.
//

#pragma once

#include <vector>
#include "Metal/Metal.h"
typedef uint32_t LibraryHandle;

class ShaderManager {
public :
    LibraryHandle loadShader(const char *path, id<MTLDevice> device );
    id<MTLLibrary> getLibraryFromHandle(LibraryHandle handle);

private:
    std::vector<id> m_shaders;
};



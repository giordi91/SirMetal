#pragma once

#import <Metal/Metal.h>

namespace SirMetal {

    struct AlphaBlendingState
    {
        bool enabled = false;
        MTLBlendOperation rgbBlendOperation;
        MTLBlendOperation alphaBlendOperation;
        MTLBlendFactor sourceRGBBlendFactor;
        MTLBlendFactor sourceAlphaBlendFactor;
        MTLBlendFactor destinationRGBBlendFactor;
        MTLBlendFactor destinationAlphaBlendFactor;
    };
    struct Material
    {
        std::string shaderName;
        AlphaBlendingState state;
    };


}
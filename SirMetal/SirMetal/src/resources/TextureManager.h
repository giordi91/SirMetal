#pragma once

#import <unordered_map>
#import <string>
#include <stdint.h>

#import <Metal/Metal.h>

#import "handle.h"

namespace SirMetal {
    struct AllocTextureRequest {
        uint32_t width;
        uint32_t height;
        uint32_t sampleCount;
        MTLTextureType type;
        MTLPixelFormat format;
        MTLTextureUsage usage;
        std::string name;
    };

    class TextureManager {
    public:
        TextureManager() = default;
        void initialize() {};

        TextureHandle allocate(id <MTLDevice> device, const AllocTextureRequest &request);

    private:
        struct TextureData {
            AllocTextureRequest request;
            id <MTLTexture> texture;
        };
    private:
        std::unordered_map<uint32_t, TextureData> m_data;
        std::unordered_map<std::string, uint32_t> m_nameToHandle;
        int m_textureCounter = 1;
    };
}



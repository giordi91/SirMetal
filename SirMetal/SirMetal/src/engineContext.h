#pragma once


#import <cstdint>

const char* readFile(const char* filePath);
namespace SirMetal {

    class ShaderManager;
    class TextureManager;

    struct Resources
    {
        TextureManager* textureManager = nullptr;
        ShaderManager* shaderManager = nullptr;
    };

    struct EngineContext {
        const char *projectPath;
        uint32_t screenWidth;
        uint32_t screenHeight;
        void* viewportTexture;
        Resources managers;
    };


    void initializeContext(const char *projectPath);
    extern EngineContext* CONTEXT;
}

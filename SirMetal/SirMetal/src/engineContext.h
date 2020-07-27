#pragma once

#import <cstdint>

const char *readFile(const char *filePath);

namespace SirMetal {

    class ShaderManager;

    class TextureManager;

    struct Resources {
        TextureManager *textureManager = nullptr;
        ShaderManager *shaderManager = nullptr;
    };

    enum InteractionFlagsBits {
        InteractionNone = 0,
        InteractionViewportHovered = 1
    };

    typedef uint32_t InteractionFlags ;

    struct EditorFlags {
        InteractionFlags interaction = 0;

    };


    struct EngineContext {
        const char *projectPath;
        uint32_t screenWidth;
        uint32_t screenHeight;
        void *viewportTexture;
        Resources managers;
        EditorFlags flags;
    };


    void initializeContext(const char *projectPath);

    extern EngineContext *CONTEXT;
}

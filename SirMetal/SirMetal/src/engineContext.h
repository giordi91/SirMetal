#pragma once


#import <cstdint>

const char* readFile(const char* filePath);
class ShaderManager;
namespace SirMetal {

    struct EngineContext {
        const char *projectPath;
        ShaderManager* shaderManager;
        uint32_t screenWidth;
        uint32_t screenHeight;
    };
    void initializeContext(const char *projectPath);
    extern EngineContext* CONTEXT;
}

#pragma once


const char* readFile(const char* filePath);
class ShaderManager;
namespace SirMetal {

    struct EngineContext {
        const char *projectPath;
        ShaderManager* shaderManager;
    };
    void initializeContext(const char *projectPath);
    extern EngineContext* CONTEXT;
}

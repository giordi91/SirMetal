#pragma once

const char* readFile(const char* filePath);
namespace SirMetal {

    struct EngineContext {

        const char *projectPath;
    };
    void initializeContext(const char *projectPath);
    extern EngineContext* CONTEXT;
}

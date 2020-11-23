#pragma once

#import <cstdint>
#import <objc/objc.h>
#import "SirMetalLib/graphics/renderingContext.h"
#import "graphics/camera.h"
#import "SirMetalLib/core/input.h"
#import "SirMetalLib/world/hierarchy.h"

namespace SirMetal {

    class ShaderManager;
    class TextureManager;
    class MeshManager;
    class ConstantBufferManager;

    struct World {
        Hierarchy hierarchy;
    };

    struct Resources {
        TextureManager *textureManager = nullptr;
        ShaderManager *shaderManager = nullptr;
        MeshManager *meshManager = nullptr;
        ConstantBufferManager *constantBufferManager = nullptr;
    };

    struct EngineContext {
        Resources managers;
        Input input;
        World world;
        RenderingContext renderingCtx;
        //for now the camera lives here until we start to have a more
        //proper rendering context;
        Camera camera;
        EditorFPSCameraController cameraController;
        uint32_t screenWidth;
        uint32_t screenHeight;
        uint32_t inFlightFrames = 3;
        uint32_t frame;
    };

    bool initializeContext(id device, id queue);

    void endFrame(EngineContext *context);

    extern EngineContext *CONTEXT;
}

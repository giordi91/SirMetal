#pragma once

#import <cstdint>
#import <objc/objc.h>
#import "graphics/camera.h"
#import "SirMetalLib/core/input.h"
#import "SirMetalLib/world/hierarchy.h"

const char *readFile(const char *filePath);

namespace SirMetal {

    class ShaderManager;
    class TextureManager;
    class MeshManager;

    struct World
    {
        Hierarchy hierarchy;
    };

    struct Resources {
        TextureManager *textureManager = nullptr;
        ShaderManager *shaderManager = nullptr;
        MeshManager *meshManager= nullptr;
    };

    enum InteractionFlagsBits {
        InteractionNone = 0,
        InteractionViewportFocused = 1,
        InteractionViewportGuizmo = 2,
    };

    typedef uint32_t InteractionFlags ;

    enum ViewEventsFlagsBits {
        ViewEventsNone = 0,
        ViewEventsViewportSizeChanged = 1,
    };

    typedef uint32_t ViewEventsFlags;

    struct EditorFlags {
        InteractionFlags interaction = 0;
        ViewEventsFlags viewEvents = 0;
    };


    struct EngineContext {
        uint32_t screenWidth;
        uint32_t screenHeight;
        void *viewportTexture;
        Resources managers;
        EditorFlags flags;
        Input input;
        World world;
        //for now the camera lives here until we start to have a more
        //proper rendering context;
        Camera camera;
        EditorFPSCameraController cameraController;


    };

    bool initializeContext(id device, id queue);
    void endFrame(EngineContext* context);

    extern EngineContext *CONTEXT;
}

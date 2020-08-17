#include "engineContext.h"
#include <stdio.h>
#include <stdlib.h>

#import "Metal/Metal.h"

#import "SirMetalLib/resources/shaderManager.h"
#import "SirMetalLib/resources/textureManager.h"
#import "SirMetalLib/resources/meshes/meshManager.h"
#import "SirMetalLib/core/logging/log.h"

namespace SirMetal {

    EngineContext *CONTEXT = nullptr;

    bool initializeContext(id device, id queue)
    {
        //id<MTLDevice> mtlDevice = device;

        ShaderManager* shaderManager = new ShaderManager;
        shaderManager->initialize(device);
        TextureManager* textureManager = new TextureManager;
        textureManager->initialize();
        MeshManager* meshManager= new MeshManager;
        meshManager->initialize(device,queue);

        CONTEXT = new EngineContext{ nullptr
        ,{
            textureManager,
            shaderManager,
            meshManager,
        }};
        CONTEXT->cameraController.setCamera(&CONTEXT->camera);

        return true;
    }

    void endFrame(EngineContext *context) {
        //we clear the mouse delta such that
        //if anything relies on it does not continue getting triggered
        context->input.m_mouseDeltaX = 0;
        context->input.m_mouseDeltaY = 0;
    }


}

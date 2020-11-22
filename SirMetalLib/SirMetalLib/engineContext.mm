#include "engineContext.h"
#include <stdio.h>
#include <stdlib.h>
#import <SirMetalLib/graphics/constantBufferManager.h>

#import "Metal/Metal.h"

#import "SirMetalLib/resources/shaderManager.h"
#import "SirMetalLib/resources/textureManager.h"
#import "SirMetalLib/resources/meshes/meshManager.h"
#import "SirMetalLib/core/core.h"

namespace SirMetal {

    EngineContext *CONTEXT = nullptr;
    static const uint32_t CONSTANT_BUFFER_MANAGER_ALLOC_POOL_IN_MB = 16;

    bool initializeContext(id device, id queue)
    {
        //id<MTLDevice> mtlDevice = device;

        ShaderManager* shaderManager = new ShaderManager;
        shaderManager->initialize(device);
        TextureManager* textureManager = new TextureManager;
        textureManager->initialize();
        MeshManager* meshManager= new MeshManager;
        meshManager->initialize(device,queue);
        ConstantBufferManager* constantBufferManager = new ConstantBufferManager();
        constantBufferManager->initialize(device,CONSTANT_BUFFER_MANAGER_ALLOC_POOL_IN_MB* MB_TO_BYTE);

        CONTEXT = new EngineContext{
        {
            textureManager,
            shaderManager,
            meshManager,
            constantBufferManager,
        }};
        CONTEXT->cameraController.setCamera(&CONTEXT->camera);

        return true;
    }

    void endFrame(EngineContext *context) {
        //we clear the mouse delta such that
        //if anything relies on it does not continue getting triggered
        context->input.m_mouseDeltaX = 0;
        context->input.m_mouseDeltaY = 0;
        context->frame++;
    }


}

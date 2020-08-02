#include "engineContext.h"
#include <stdio.h>
#include <stdlib.h>

#import "Metal/Metal.h"

#import "resources/shaderManager.h"
#import "resources/textureManager.h"
#import "resources/meshes/meshManager.h"
#import "log.h"
#import "project.h"

namespace SirMetal {

    EngineContext *CONTEXT = nullptr;

    bool initializeContext(id device)
    {
        //id<MTLDevice> mtlDevice = device;

        //NOTE this is something that could be fed from outside, such that we
        //are not tight to the project and editor namespace, say in the case
        //we deploy a game
        const std::string& projectPath = SirMetal::Editor::PROJECT->getProjectPath();

        ShaderManager* shaderManager = new ShaderManager;
        shaderManager->initialize(device,projectPath);
        TextureManager* textureManager = new TextureManager;
        textureManager->initialize();
        MeshManager* meshManager= new MeshManager;
        meshManager->initialize(device);

        CONTEXT = new EngineContext{ 0,0, nullptr
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
#include "engineContext.h"
#include <stdio.h>
#include <stdlib.h>

const char* readFile(const char* filePath) {




/* declare a file pointer */
    FILE    *infile;
    char    *buffer;
    long    numbytes;

/* open an existing file for reading */
    infile = fopen("test.rib", "r");

/* quit if the file does not exist */
    if(infile == NULL)
        return nullptr;

/* Get the number of bytes */
    fseek(infile, 0L, SEEK_END);
    numbytes = ftell(infile);

/* reset the file position indicator to
the beginning of the file */
    fseek(infile, 0L, SEEK_SET);

/* grab sufficient memory for the
buffer to hold the text */
    buffer = (char*)new char[numbytes];

/* memory error */
    if(buffer == NULL)
        return nullptr;

/* copy all the text into the buffer */
    fread(buffer, sizeof(char), numbytes, infile);
    fclose(infile);
    return buffer;
}
#import "Metal/Metal.h"

#import "resources/shaderManager.h"
#import "resources/textureManager.h"
#import "log.h"
#import "project.h"

namespace SirMetal {

    EngineContext *CONTEXT = nullptr;

    bool initializeContext()
    {

        //NOTE this is something that could be fed from outside, such that we
        //are not tight to the project and editor namespace, say in the case
        //we deploy a game
        const std::string& projectPath = SirMetal::Editor::PROJECT->getProjectPath();

        ShaderManager* shaderManager = new ShaderManager;
        shaderManager->initialize(projectPath);
        TextureManager* textureManager = new TextureManager;
        textureManager->initialize();

        CONTEXT = new EngineContext{ 0,0, nullptr
        ,{
            textureManager,
            shaderManager,
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
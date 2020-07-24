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

#import "shaderManager.h"

namespace SirMetal {

    EngineContext *CONTEXT = nullptr;

    void initializeContext(const char *projectPath)
    {
        printf("Initializing sir metal project at: %s\n", projectPath);

        ShaderManager* shaderManager = new ShaderManager;
        CONTEXT = new EngineContext{projectPath, shaderManager};
    }


}
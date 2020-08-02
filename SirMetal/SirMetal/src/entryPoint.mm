
#include "entryPoint.h"
#import "log.h"
#import "project.h"
#import "engineContext.h"

namespace SirMetal {
    bool startup(id device) {
        bool result = Editor::initializeProject();
        if(!result ){return false;}
        result = initializeContext(device);
        if(!result ){return false;}
        result = Editor::PROJECT->processProjectAssets();
        return result;

    }


    void shutdown() {

        Editor::PROJECT->save();


        delete CONTEXT;
        delete Editor::PROJECT;
        Log::free();
    }
}

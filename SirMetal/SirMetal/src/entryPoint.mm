
#include "entryPoint.h"
#import "log.h"
#import "project.h"
#import "engineContext.h"

namespace SirMetal {
    bool startup() {
        bool result = Editor::initializeProject();
        if(!result ){return false;}
        result = initializeContext();
        if(!result ){return false;}
        return true;

    }

    void shutdown() {

        Editor::PROJECT->save();


        delete CONTEXT;
        delete Editor::PROJECT;
        Log::free();
    }
}

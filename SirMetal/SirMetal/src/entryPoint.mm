
#include "entryPoint.h"
#import "log.h"
#import "project.h"
#import "engineContext.h"

namespace SirMetal {
    void startup() {
        Log::init();
        Editor::initializeProject();
        initializeContext();
    }

    void shutdown() {
        delete CONTEXT;
        delete Editor::PROJECT;
    }
}

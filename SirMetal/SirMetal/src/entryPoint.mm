
#include "entryPoint.h"
#import "SirMetalLib/core/logging/log.h"
#import "SirMetalLib/engineContext.h"

namespace SirMetal {
    bool startup(id device, id queue) {

        Log::init("");
        bool result = initializeContext(device,queue);
        if(!result ){return false;}
        return result;
    }


    void shutdown() {
        delete CONTEXT;
        Log::free();
    }
}

#pragma once

#import <objc/objc.h>

namespace SirMetal {
    bool startup(id device, id queue);

    void shutdown();
}

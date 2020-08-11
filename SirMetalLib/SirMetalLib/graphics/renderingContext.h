#pragma once


#import <objc/objc.h>

namespace  SirMetal{

    //This class is intended to keep track of the state
    //of the frame, what render target are bounds etc
    struct DrawTracker
    {
        id renderTargets[8];
        id depthTarget;
    };

}

#pragma once


#import <objc/objc.h>

namespace  SirMetal{

    static const uint MAX_COLOR_ATTACHMENT = 8;
    //This class is intended to keep track of the state
    //of the frame, what render target are bounds etc
    struct DrawTracker
    {
        id renderTargets[8];
        id depthTarget;
        void reset()
        {
            for (int i = 0; i < SirMetal::MAX_COLOR_ATTACHMENT; ++i) {
                renderTargets[i] = nil;
            }
            depthTarget = nil;
        }
    };


}

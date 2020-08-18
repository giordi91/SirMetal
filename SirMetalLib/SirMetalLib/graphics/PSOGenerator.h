#pragma once

#import <objc/objc.h>

namespace SirMetal {
struct PSOCache {
  id color = nil;
  id depth = nil;
};

PSOCache getPSO(id device, const SirMetal::DrawTracker &tracker, const SirMetal::Material &material);

}

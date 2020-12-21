#pragma once

#include "renderingContext.h"
#import <objc/objc.h>

namespace SirMetal {
struct EngineContext;
namespace graphics {
struct DrawTracker;
}
struct Material;
struct PSOCache {
  id color = nil;
  id depth = nil;
  id vtxFunction;
  id fragFunction;
};

PSOCache getPSO(EngineContext*context, const SirMetal::graphics::DrawTracker &tracker,
                const SirMetal::Material &material);

} // namespace SirMetal

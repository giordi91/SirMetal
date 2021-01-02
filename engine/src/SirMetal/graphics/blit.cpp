#include "SirMetal/graphics/blit.h"

#include "SirMetal/engine.h"
#include "SirMetal/graphics/PSOGenerator.h"
#include "SirMetal/graphics/materialManager.h"
#include "SirMetal/resources/textureManager.h"

namespace SirMetal::graphics {

void doBlit(EngineContext *context, id<MTLRenderCommandEncoder> encoder,
            const BlitRequest &request) {
  SirMetal::graphics::DrawTracker tracker{};
  tracker.renderTargets[0] = request.dstTexture;
  tracker.depthTarget = {};

  SirMetal::PSOCache cache =
          getPSO(context, tracker, SirMetal::Material{"fullscreen", false});

  id<MTLTexture> colorTexture =
          context->m_textureManager->getNativeFromHandle(request.srcTexture);

  if (request.customScissor) {
    MTLScissorRect rect;
    rect.width = request.scissor[0];
    rect.height = request.scissor[1];
    rect.x = request.scissor[2];
    rect.y = request.scissor[3];
    [encoder setScissorRect:rect];
  }
  if (request.customView) {
    MTLViewport view;
    view.width = request.viewport[0];
    view.height = request.viewport[1];
    view.originX = request.viewport[2];
    view.originY = request.viewport[3];
    view.znear = request.viewport[4];
    view.zfar = request.viewport[5];
    [encoder setViewport:view];
  }


  [encoder setRenderPipelineState:cache.color];
  [encoder setFrontFacingWinding:MTLWindingCounterClockwise];
  [encoder setCullMode:MTLCullModeBack];
  [encoder setFragmentTexture:colorTexture atIndex:0];
  [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
}
}// namespace SirMetal::graphics
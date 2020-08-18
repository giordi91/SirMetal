#include "SirMetalLib/graphics/techniques/selection.h"

#import <Metal/Metal.h>
#import <SirMetalLib/core/memory/denseTree.h>
#import <SirMetalLib/engineContext.h>
#import <SirMetalLib/graphics/constantBufferManager.h>
#import <SirMetalLib/resources/meshes/meshManager.h>

namespace SirMetal
{

void Selection::initialize() {

}
void Selection::render(id commandBuffer, int screenWidth, int screenHeight) {
  /*
  MTLRenderPassDescriptor *passDescriptor = [[MTLRenderPassDescriptor alloc] init];
  MTLRenderPassColorAttachmentDescriptor *uiColorAttachment = passDescriptor.colorAttachments[0];
  uiColorAttachment.texture = m_jumpMaskTexture;
  uiColorAttachment.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
  uiColorAttachment.storeAction = MTLStoreActionStore;
  uiColorAttachment.loadAction = MTLLoadActionClear;
  passDescriptor.renderTargetWidth = w;
  passDescriptor.renderTargetHeight = h;

  id <MTLRenderCommandEncoder> renderPass = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];


  //TODO use a reset function on tracker
  tracker->depthTarget = nil;
  tracker->renderTargets[0] = m_jumpMaskTexture;
  PSOCache cache = getPSO(_device, *tracker, m_jumpFloodMaskMaterial);

  //[renderPass setRenderPipelineState:self.jumpMaskPipelineState];
  [renderPass setRenderPipelineState:cache.color];
  //[renderPass setDepthStencilState:nil];
  [renderPass setFrontFacingWinding:MTLWindingCounterClockwise];
  [renderPass setCullMode:MTLCullModeBack];

  auto* cbManager = SirMetal::CONTEXT->managers.constantBufferManager;
  SirMetal::BindInfo  bindInfo = cbManager->getBindInfo(m_uniformHandle);

  [renderPass setVertexBuffer:bindInfo.buffer offset:bindInfo.offset atIndex:1];

  int selectedId = SirMetal::CONTEXT->world.hierarchy.getSelectedId();
  SirMetal::DenseTreeNode& selectedNode= SirMetal::CONTEXT->world.hierarchy.getNodes()[selectedId];
  SirMetal::MeshHandle mesh{selectedNode.id};
  //SirMetal::MeshHandle mesh = SirMetal::CONTEXT->managers.meshManager->getHandleFromName("lucy");
  const SirMetal::MeshData *meshData = SirMetal::CONTEXT->managers.meshManager->getMeshData(mesh);

  [renderPass setVertexBuffer:meshData->vertexBuffer offset:0 atIndex:0];
  [renderPass drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                         indexCount:meshData->primitivesCount
                          indexType:MBEIndexType
                        indexBuffer:meshData->indexBuffer
                  indexBufferOffset:0];

  [renderPass endEncoding];

  //now do the position render
  passDescriptor = [[MTLRenderPassDescriptor alloc] init];
  MTLRenderPassColorAttachmentDescriptor *colorAttachment = passDescriptor.colorAttachments[0];
  colorAttachment.texture = self.jumpTexture;
  colorAttachment.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
  colorAttachment.storeAction = MTLStoreActionStore;
  colorAttachment.loadAction = MTLLoadActionClear;
  passDescriptor.renderTargetWidth = w;
  passDescriptor.renderTargetHeight = h;

  id <MTLRenderCommandEncoder> renderPass2 = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];

  tracker->depthTarget = nil;
  tracker->renderTargets[0] = self.jumpTexture;
  cache = getPSO(_device, *tracker, m_jumpFloodInitMaterial);
  [renderPass2 setRenderPipelineState:cache.color];
  [renderPass2 setFrontFacingWinding:MTLWindingClockwise];
  [renderPass2 setCullMode:MTLCullModeNone];
  [renderPass2 setFragmentTexture:self.jumpMaskTexture atIndex:0];

  //first screen pass
  [renderPass2 drawPrimitives:MTLPrimitiveTypeTriangle
                  vertexStart:0 vertexCount:6];


  [renderPass2 endEncoding];

  //prepare render pass

  //compute offset for jump flooding
  int passIndex = 0;
  int N = 64;
  int offset = 9999;
  id <MTLTexture> outTex = m_jumpTexture;
  tracker->depthTarget = nil;
  tracker->renderTargets[0] = m_jumpTexture;
  cache = getPSO(_device, *tracker, m_jumpFloodMaterial);

  while (offset != 1) {
    offset = static_cast<int>(pow(2, (log2(N) - passIndex - 1)));

    //do render here
    MTLRenderPassDescriptor *passDescriptorP = [[MTLRenderPassDescriptor alloc] init];
    colorAttachment = passDescriptorP.colorAttachments[0];
    colorAttachment.texture = passIndex % 2 == 0 ? m_jumpTexture2 : m_jumpTexture;
    colorAttachment.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    colorAttachment.storeAction = MTLStoreActionStore;
    colorAttachment.loadAction = MTLLoadActionClear;
    passDescriptorP.renderTargetWidth = static_cast<NSUInteger>(w);
    passDescriptorP.renderTargetHeight = static_cast<NSUInteger>(h);

    id <MTLRenderCommandEncoder> renderPass3 = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptorP];

    [renderPass3 setRenderPipelineState:cache.color];
    [renderPass3 setFrontFacingWinding:MTLWindingClockwise];
    [renderPass3 setCullMode:MTLCullModeNone];
    [renderPass3 setFragmentTexture:passIndex % 2 == 0 ? m_jumpTexture : m_jumpTexture2 atIndex:0];
    //TODO fix hardcoded offset
    [renderPass3 setFragmentBuffer:self.floodUniform offset:10 * 256 + passIndex * 256 atIndex:0];


    //first screen pass
    [renderPass3 drawPrimitives:MTLPrimitiveTypeTriangle
                    vertexStart:0 vertexCount:6];

    [renderPass3 endEncoding];
    outTex = passIndex % 2 == 0 ? m_jumpTexture2 : m_jumpTexture;
    passIndex += 1;
  }

  //render outline
  //now do the position render
  passDescriptor = [[MTLRenderPassDescriptor alloc] init];
  colorAttachment = passDescriptor.colorAttachments[0];
  colorAttachment.texture = self.offScreenTexture;
  colorAttachment.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
  colorAttachment.storeAction = MTLStoreActionStore;
  colorAttachment.loadAction = MTLLoadActionLoad;
  passDescriptor.renderTargetWidth = static_cast<NSUInteger>(w);
  passDescriptor.renderTargetHeight = static_cast<NSUInteger>(h);

  id <MTLRenderCommandEncoder> renderPass4 = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];
  tracker->depthTarget = nil;
  tracker->renderTargets[0] = self.offScreenTexture;
  cache = getPSO(_device, *tracker, m_jumpOutlineMaterial);

  [renderPass4 setRenderPipelineState:cache.color];
  [renderPass4 setFrontFacingWinding:MTLWindingClockwise];
  [renderPass4 setCullMode:MTLCullModeNone];
  [renderPass4 setFragmentTexture:m_jumpMaskTexture atIndex:0];
  [renderPass4 setFragmentTexture:outTex atIndex:1];
  //we need the screen size from the uniform
  [renderPass4 setFragmentBuffer:self.floodUniform offset:0 atIndex:0];

  [renderPass4 drawPrimitives:MTLPrimitiveTypeTriangle
                  vertexStart:0 vertexCount:6];
  [renderPass4 endEncoding];
   */

}
}
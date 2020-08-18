#include "SirMetalLib/graphics/techniques/selectionRendering.h"

#import <Metal/Metal.h>
#import <SirMetalLib/core/memory/denseTree.h>
#import <SirMetalLib/engineContext.h>
#import <SirMetalLib/graphics/constantBufferManager.h>
#import <SirMetalLib/resources/meshes/meshManager.h>
#import <SirMetalLib/graphics/renderingContext.h>
#import <SirMetalLib/graphics/PSOGenerator.h>
#import <SirMetalLib/resources/textureManager.h>
#import <SirMetalLib/core/logging/log.h>

namespace SirMetal {

void updateFloodIndices(int w, int h, ConstantBufferHandle buffer) {
  std::vector<int> indices;
  indices.resize(256 / 4 * 16);
  int N = static_cast<int>(pow(2, 16));
  for (int i = 0; i < 16; ++i) {
    int offset = static_cast<int>(pow(2, (log2(N) - i - 1)));
    int id = i * 64;
    indices[id] = offset;
    indices[id + 1] = w;
    indices[id + 2] = h;
  }
  ConstantBufferManager *cbManager = CONTEXT->managers.constantBufferManager;
  cbManager->update(buffer, indices.data());

}

void Selection::initialize(id inDevice) {

  id<MTLDevice> device = inDevice;
  m_jumpFloodInitMaterial.shaderName = "jumpInit";
  m_jumpFloodInitMaterial.state.enabled = false;
  m_jumpFloodMaskMaterial.shaderName = "jumpMask";
  m_jumpFloodMaskMaterial.state.enabled = false;
  m_jumpFloodMaterial.shaderName = "jumpFlood";
  m_jumpFloodMaterial.state.enabled = false;
  m_jumpOutlineMaterial.shaderName = "jumpOutline";
  m_jumpOutlineMaterial.state.enabled = true;
  SirMetal::AlphaBlendingState &alpha = m_jumpOutlineMaterial.state;
  alpha.rgbBlendOperation = MTLBlendOperationAdd;
  alpha.alphaBlendOperation = MTLBlendOperationAdd;
  alpha.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
  alpha.sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
  alpha.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
  alpha.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

  //allocating 16 values, then just moving the offset when binding, now
  //i am not sure what is the minimum stride so I am going for a 32*4 bits
  int sizeFlood = 256 * 16;
  ConstantBufferManager *cbManager = CONTEXT->managers.constantBufferManager;
  m_floodCB = cbManager->allocate(sizeFlood, CONSTANT_BUFFER_FLAG_NONE);
  updateFloodIndices(m_screenWidth, m_screenHeight, m_floodCB);

  SirMetal::TextureManager *textureManager = SirMetal::CONTEXT->managers.textureManager;
  SirMetal::AllocTextureRequest jumpRequest{
      static_cast<uint32_t>(m_screenWidth), static_cast<uint32_t>(m_screenHeight),
      1, MTLTextureType2D, MTLPixelFormatRG32Float,
      MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead, MTLStorageModePrivate, 1, "jumpFloodTexture"
  };
  m_jumpHandle = textureManager->allocate(device, jumpRequest);
  m_jumpTexture = textureManager->getNativeFromHandle(m_jumpHandle);
  //second texture used to do ping pong for selection
  jumpRequest.name = "jumpRequest2";
  m_jumpHandle2 = textureManager->allocate(device, jumpRequest);
  m_jumpTexture2 = textureManager->getNativeFromHandle(m_jumpHandle2);

  SirMetal::AllocTextureRequest jumpMaskRequest{
      static_cast<uint32_t>(m_screenWidth), static_cast<uint32_t>(m_screenHeight),
      1, MTLTextureType2D, MTLPixelFormatR8Unorm,
      MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead, MTLStorageModePrivate, 1, "jumpFloodMaskTexture"
  };
  m_jumpMaskHandle = textureManager->allocate(device, jumpMaskRequest);
  m_jumpMaskTexture = textureManager->getNativeFromHandle(m_jumpMaskHandle);

}
void Selection::render(id inDevice,
                       id commandBuffer,
                       int screenWidth,
                       int screenHeight,
                       ConstantBufferHandle uniformHandle,
                       id offscreenTexture) {

  id<MTLDevice> device = inDevice;

  if (screenHeight != m_screenHeight || screenWidth != m_screenWidth) {
    SirMetal::TextureManager *texManager = SirMetal::CONTEXT->managers.textureManager;
    bool jumpResult = texManager->resizeTexture(device, m_jumpHandle, screenWidth, screenHeight);
    bool jumpResult2 = texManager->resizeTexture(device, m_jumpHandle2, screenWidth, screenHeight);
    bool jumpMaskResult = texManager->resizeTexture(device, m_jumpMaskHandle, screenWidth, screenHeight);
    if ((!jumpResult) | (!jumpResult2) | (!jumpMaskResult)) {
      SIR_CORE_FATAL("Could not resize selection textures");
    } else {
      m_jumpTexture = texManager->getNativeFromHandle(m_jumpHandle);
      m_jumpTexture2 = texManager->getNativeFromHandle(m_jumpHandle2);
      m_jumpMaskTexture = texManager->getNativeFromHandle(m_jumpMaskHandle);
      updateFloodIndices(screenWidth, screenHeight, m_floodCB);
    }
    m_screenHeight = static_cast<uint32_t>(screenHeight);
    m_screenWidth = static_cast<uint32_t>(screenWidth);
  }

  MTLRenderPassDescriptor *passDescriptor = [[MTLRenderPassDescriptor alloc] init];
  MTLRenderPassColorAttachmentDescriptor *uiColorAttachment = passDescriptor.colorAttachments[0];
  uiColorAttachment.texture = m_jumpMaskTexture;
  uiColorAttachment.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
  uiColorAttachment.storeAction = MTLStoreActionStore;
  uiColorAttachment.loadAction = MTLLoadActionClear;
  passDescriptor.renderTargetWidth = static_cast<NSUInteger>(screenWidth);
  passDescriptor.renderTargetHeight = static_cast<NSUInteger>(screenHeight);

  id<MTLRenderCommandEncoder> renderPass = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];


  DrawTracker tracker{};
  tracker.depthTarget = nil;
  tracker.renderTargets[0] = m_jumpMaskTexture;
  PSOCache cache = getPSO(device, tracker, m_jumpFloodMaskMaterial);

  [renderPass setRenderPipelineState:cache.color];
  [renderPass setFrontFacingWinding:MTLWindingCounterClockwise];
  [renderPass setCullMode:MTLCullModeBack];

  auto *cbManager = SirMetal::CONTEXT->managers.constantBufferManager;
  SirMetal::BindInfo bindInfo = cbManager->getBindInfo(uniformHandle);

  [renderPass setVertexBuffer:bindInfo.buffer offset:bindInfo.offset atIndex:1];

  int selectedId = SirMetal::CONTEXT->world.hierarchy.getSelectedId();
  SirMetal::DenseTreeNode &selectedNode = SirMetal::CONTEXT->world.hierarchy.getNodes()[selectedId];
  SirMetal::MeshHandle mesh{selectedNode.id};
  //TODO need to get not only the mesh but corresponding uniform buffer
  const SirMetal::MeshData *meshData = SirMetal::CONTEXT->managers.meshManager->getMeshData(mesh);

  [renderPass setVertexBuffer:meshData->vertexBuffer offset:0 atIndex:0];
  [renderPass drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                         indexCount:meshData->primitivesCount
                          indexType:MTLIndexTypeUInt32
                        indexBuffer:meshData->indexBuffer
                  indexBufferOffset:0];

  [renderPass endEncoding];

  //now do the position render
  passDescriptor = [[MTLRenderPassDescriptor alloc] init];
  MTLRenderPassColorAttachmentDescriptor *colorAttachment = passDescriptor.colorAttachments[0];
  colorAttachment.texture = m_jumpTexture;
  colorAttachment.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
  colorAttachment.storeAction = MTLStoreActionStore;
  colorAttachment.loadAction = MTLLoadActionClear;
  passDescriptor.renderTargetWidth = static_cast<NSUInteger>(screenWidth);
  passDescriptor.renderTargetHeight = static_cast<NSUInteger>(screenHeight);

  id<MTLRenderCommandEncoder> renderPass2 = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];

  tracker.depthTarget = nil;
  tracker.renderTargets[0] = m_jumpTexture;
  cache = getPSO(device, tracker, m_jumpFloodInitMaterial);
  [renderPass2 setRenderPipelineState:cache.color];
  [renderPass2 setFrontFacingWinding:MTLWindingClockwise];
  [renderPass2 setCullMode:MTLCullModeNone];
  [renderPass2 setFragmentTexture:m_jumpMaskTexture atIndex:0];

  //first screen pass
  [renderPass2 drawPrimitives:MTLPrimitiveTypeTriangle
                  vertexStart:0 vertexCount:6];

  [renderPass2 endEncoding];

  //compute offset for jump flooding
  int passIndex = 0;
  //TODO hardcoded width we should get this from the settings
  int N = 64;
  //we compute the offset up to 2^16, and we lay them in a buffer
  //so instead of updating the buffer at every iteration, we simply
  //bind with an offset and avoid the updates. since the buffer beings
  //with 2^16 and goes down, we need to compute how far down the buffer we have to go
  //and is the difference between the log 2 of the max offset minus the log2 of the offset
  //width we are using now
  int maxOffsetPower = 16;
  int baseOffsetPower = static_cast<int>(log2(64));
  int uniformBufferShift = maxOffsetPower - baseOffsetPower;
  int offset = 9999;
  id<MTLTexture> outTex = m_jumpTexture;
  tracker.depthTarget = nil;
  tracker.renderTargets[0] = m_jumpTexture;
  cache = getPSO(device, tracker, m_jumpFloodMaterial);

  auto floodUniform = cbManager->getBindInfo(m_floodCB);
  while (offset != 1) {
    offset = static_cast<int>(pow(2, (log2(N) - passIndex - 1)));

    //do render here
    MTLRenderPassDescriptor *passDescriptorP = [[MTLRenderPassDescriptor alloc] init];
    colorAttachment = passDescriptorP.colorAttachments[0];
    colorAttachment.texture = passIndex % 2 == 0 ? m_jumpTexture2 : m_jumpTexture;
    colorAttachment.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    colorAttachment.storeAction = MTLStoreActionStore;
    colorAttachment.loadAction = MTLLoadActionClear;
    passDescriptorP.renderTargetWidth = static_cast<NSUInteger>(screenWidth);
    passDescriptorP.renderTargetHeight = static_cast<NSUInteger>(screenHeight);

    id<MTLRenderCommandEncoder> renderPass3 = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptorP];

    [renderPass3 setRenderPipelineState:cache.color];
    [renderPass3 setFrontFacingWinding:MTLWindingClockwise];
    [renderPass3 setCullMode:MTLCullModeNone];
    [renderPass3 setFragmentTexture:passIndex % 2 == 0 ? m_jumpTexture : m_jumpTexture2 atIndex:0];

    //the constant buffer are pooled, sub allocations of bigger buffers, so we have
    //a basic allocation buffer, then we have our uniformBuffer shift which tells us
    //how many "slots" in the buffer we have to jump, the slot itself would be very small,
    //3 integers, but of course we have alignment requirement which is 256 so we offset by that
    uint32_t bufferOffset = floodUniform.offset + (uniformBufferShift + passIndex) * 256;
    [renderPass3 setFragmentBuffer:floodUniform.buffer offset: bufferOffset atIndex:0];


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
  colorAttachment.texture = offscreenTexture;
  colorAttachment.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
  colorAttachment.storeAction = MTLStoreActionStore;
  colorAttachment.loadAction = MTLLoadActionLoad;
  passDescriptor.renderTargetWidth = static_cast<NSUInteger>(screenWidth);
  passDescriptor.renderTargetHeight = static_cast<NSUInteger>(screenHeight);

  id<MTLRenderCommandEncoder> renderPass4 = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];
  tracker.depthTarget = nil;
  tracker.renderTargets[0] = offscreenTexture;
  cache = getPSO(device, tracker, m_jumpOutlineMaterial);

  [renderPass4 setRenderPipelineState:cache.color];
  [renderPass4 setFrontFacingWinding:MTLWindingClockwise];
  [renderPass4 setCullMode:MTLCullModeNone];
  [renderPass4 setFragmentTexture:m_jumpMaskTexture atIndex:0];
  [renderPass4 setFragmentTexture:outTex atIndex:1];
  //we need the screen size from the uniform
  [renderPass4 setFragmentBuffer:floodUniform.buffer offset:floodUniform.offset atIndex:0];

  [renderPass4 drawPrimitives:MTLPrimitiveTypeTriangle
                  vertexStart:0 vertexCount:6];
  [renderPass4 endEncoding];

}
}
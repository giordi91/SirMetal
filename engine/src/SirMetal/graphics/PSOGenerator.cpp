
#include "PSOGenerator.h"
#import "SirMetal/engine.h"
#import "SirMetal/graphics/materialManager.h"
#import "SirMetal/graphics/renderingContext.h"
#import "SirMetal/resources/handle.h"
#import "SirMetal/resources/shaderManager.h"
#import <Metal/Metal.h>
#import <cstddef>
#import <unordered_map>

namespace SirMetal {

static std::unordered_map<std::size_t, PSOCache> m_psoCache;
static const uint MAX_COLOR_ATTACHMENT = 8;

template <class T> constexpr void hash_combine(std::size_t &seed, const T &v) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

std::size_t computePSOHash(const SirMetal::graphics::DrawTracker &tracker,
                           const Material &material) {
  std::size_t toReturn = 0;

  for (int i = 0; i < MAX_COLOR_ATTACHMENT; ++i) {
    if (tracker.renderTargets[i] != nil) {
      id<MTLTexture> tex = tracker.renderTargets[i];
      hash_combine(toReturn, static_cast<int>(tex.pixelFormat));
    }
  }
  if (tracker.depthTarget != nil) {
    id<MTLTexture> tex = tracker.depthTarget;
    hash_combine(toReturn, static_cast<int>(tex.pixelFormat));
  }

  if (material.blendingState.enabled) {
    hash_combine(toReturn, material.blendingState.rgbBlendOperation);
    hash_combine(toReturn, material.blendingState.alphaBlendOperation);
    hash_combine(toReturn, material.blendingState.sourceRGBBlendFactor);
    hash_combine(toReturn, material.blendingState.sourceAlphaBlendFactor);
    hash_combine(toReturn, material.blendingState.destinationRGBBlendFactor);
    hash_combine(toReturn, material.blendingState.destinationAlphaBlendFactor);
  }

  hash_combine(toReturn, material.shaderName);
  return toReturn;
}

PSOCache getPSO(EngineContext *context,
                const SirMetal::graphics::DrawTracker &tracker,
                const SirMetal::Material &material) {

  id<MTLDevice> device = context->m_renderingContext->getDevice();
  std::size_t hash = computePSOHash(tracker, material);
  auto found = m_psoCache.find(hash);
  if (found != m_psoCache.end()) {
    return found->second;
  }
  SirMetal::ShaderManager *sManager = context->m_shaderManager;
  SirMetal::LibraryHandle lh = sManager->getHandleFromName(material.shaderName);

  MTLRenderPipelineDescriptor *pipelineDescriptor =
      [MTLRenderPipelineDescriptor new];
  pipelineDescriptor.vertexFunction = sManager->getVertexFunction(lh);
  pipelineDescriptor.fragmentFunction = sManager->getFragmentFunction(lh);


  for (int i = 0; i < MAX_COLOR_ATTACHMENT; ++i) {
    id<MTLTexture> texture = tracker.renderTargets[i];
    pipelineDescriptor.colorAttachments[i].pixelFormat = texture.pixelFormat;
  }

  // NOTE for now we will use the albpla blending only for the first color
  // attachment need to figure out a way to let the user specify per render
  // target maybe?
  if (material.blendingState.enabled) {
    pipelineDescriptor.colorAttachments[0].blendingEnabled = YES;
    pipelineDescriptor.colorAttachments[0].rgbBlendOperation =
        material.blendingState.rgbBlendOperation;
    pipelineDescriptor.colorAttachments[0].alphaBlendOperation =
        material.blendingState.alphaBlendOperation;
    pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor =
        material.blendingState.sourceRGBBlendFactor;
    pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor =
        material.blendingState.sourceAlphaBlendFactor;
    pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor =
        material.blendingState.destinationAlphaBlendFactor;
    pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor =
        material.blendingState.destinationAlphaBlendFactor;
  }

  PSOCache cache{nil, nil, pipelineDescriptor.vertexFunction,pipelineDescriptor.fragmentFunction};

  if (tracker.depthTarget != nil) {
    // need to do depth stuff here
    id<MTLTexture> texture = tracker.depthTarget;
    pipelineDescriptor.depthAttachmentPixelFormat = texture.pixelFormat;
    MTLDepthStencilDescriptor *depthStencilDescriptor =
        [MTLDepthStencilDescriptor new];
    depthStencilDescriptor.depthCompareFunction = MTLCompareFunctionLess;
    depthStencilDescriptor.depthWriteEnabled = YES;
    cache.depth =
        [device newDepthStencilStateWithDescriptor:depthStencilDescriptor];
  }

  NSError *error = nullptr;
  cache.color = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor
                                                       error:&error];

  if (error) {
    NSLog(@"Failed to create pipeline state: %@", error);
  }
  m_psoCache[hash] = cache;
  return cache;
}

} // namespace SirMetal
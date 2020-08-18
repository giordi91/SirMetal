//
// Created by Marco Giordano on 18/08/2020.
// Copyright (c) 2020 Marco Giordano. All rights reserved.
//

#import <cstddef>
#import <Metal/Metal.h>
#import <unordered_map>
#import "SirMetalLib/graphics/renderingContext.h"
#import "SirMetalLib/engineContext.h"
#import "SirMetalLib/graphics/materialManager.h"
#import "SirMetalLib/resources/handle.h"
#import "SirMetalLib/resources/shaderManager.h"
#include "PSOGenerator.h"

namespace SirMetal {

static std::unordered_map<std::size_t, PSOCache> m_psoCache;

template<class T>
constexpr void hash_combine(std::size_t &seed, const T &v) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

std::size_t computePSOHash(const SirMetal::DrawTracker &tracker, const SirMetal::Material &material) {
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

  if (material.state.enabled) {
    hash_combine(toReturn, material.state.rgbBlendOperation);
    hash_combine(toReturn, material.state.alphaBlendOperation);
    hash_combine(toReturn, material.state.sourceRGBBlendFactor);
    hash_combine(toReturn, material.state.sourceAlphaBlendFactor);
    hash_combine(toReturn, material.state.destinationRGBBlendFactor);
    hash_combine(toReturn, material.state.destinationAlphaBlendFactor);
  }

  hash_combine(toReturn, material.shaderName);
  return toReturn;
}

PSOCache getPSO(id device, const SirMetal::DrawTracker &tracker, const SirMetal::Material &material) {

  std::size_t hash = computePSOHash(tracker, material);
  auto found = m_psoCache.find(hash);
  if (found != m_psoCache.end()) {
    return found->second;
  }
  SirMetal::ShaderManager *sManager = SirMetal::CONTEXT->managers.shaderManager;
  SirMetal::LibraryHandle lh = sManager->getHandleFromName(material.shaderName);

  MTLRenderPipelineDescriptor *pipelineDescriptor = [MTLRenderPipelineDescriptor new];
  pipelineDescriptor.vertexFunction = sManager->getVertexFunction(lh);
  pipelineDescriptor.fragmentFunction = sManager->getFragmentFunction(lh);

  for (int i = 0; i < MAX_COLOR_ATTACHMENT; ++i) {
    id<MTLTexture> texture = tracker.renderTargets[i];
    pipelineDescriptor.colorAttachments[i].pixelFormat = texture.pixelFormat;
  }

  //NOTE for now we will use the albpla blending only for the first color attachment
  //need to figure out a way to let the user specify per render target maybe?
  if (material.state.enabled) {
    pipelineDescriptor.colorAttachments[0].blendingEnabled = YES;
    pipelineDescriptor.colorAttachments[0].rgbBlendOperation = material.state.rgbBlendOperation;
    pipelineDescriptor.colorAttachments[0].alphaBlendOperation = material.state.alphaBlendOperation;
    pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = material.state.sourceRGBBlendFactor;
    pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = material.state.sourceAlphaBlendFactor;
    pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = material.state.destinationAlphaBlendFactor;
    pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = material.state.destinationAlphaBlendFactor;
  }

  PSOCache cache{nil, nil};

  if (tracker.depthTarget != nil) {
    //need to do depth stuff here
    id<MTLTexture> texture = tracker.depthTarget;
    pipelineDescriptor.depthAttachmentPixelFormat = texture.pixelFormat;
    MTLDepthStencilDescriptor *depthStencilDescriptor = [MTLDepthStencilDescriptor new];
    depthStencilDescriptor.depthCompareFunction = MTLCompareFunctionLess;
    depthStencilDescriptor.depthWriteEnabled = YES;
    cache.depth = [device newDepthStencilStateWithDescriptor:depthStencilDescriptor];
  }

  NSError *error = NULL;
  cache.color = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor
                                                       error:&error];
  m_psoCache[hash] = cache;
  return cache;
}

}
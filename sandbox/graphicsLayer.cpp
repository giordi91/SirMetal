#include "graphicsLayer.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <SirMetal/MBEMathUtilities.h>
#include <SirMetal/resources/textureManager.h>
#include <simd/simd.h>

#include "SirMetal/application/window.h"
#include "SirMetal/core/input.h"
#include "SirMetal/engine.h"
#include "SirMetal/graphics/constantBufferManager.h"
#include "SirMetal/graphics/materialManager.h"
#include "SirMetal/graphics/renderingContext.h"
#include "SirMetal/resources/meshes/meshManager.h"
#include "SirMetal/resources/shaderManager.h"

struct PSOCache {
  id<MTLRenderPipelineState> color = nil;
  id<MTLDepthStencilState> depth = nil;
};

static std::unordered_map<std::size_t, PSOCache> m_psoCache;
static const uint MAX_COLOR_ATTACHMENT = 8;

typedef struct {
  matrix_float4x4 modelViewProjectionMatrix;
} MBEUniforms;

static inline uint64_t AlignUp(uint64_t n, uint32_t alignment) {
  return ((n + alignment - 1) / alignment) * alignment;
}
static const NSInteger bufferIndex = 0;

namespace Sandbox {
void GraphicsLayer::onAttach(SirMetal::EngineContext *context) {

  m_engine = context;
  // initializing the camera to the identity
  m_camera.viewMatrix = matrix_float4x4_translation(vector_float3{0, 0, 0});
  m_camera.fov = M_PI / 4;
  m_camera.nearPlane = 1;
  m_camera.farPlane = 100;
  m_cameraController.setCamera(&m_camera);
  m_cameraController.setPosition(0, 0, 5);
  m_camConfig = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.2, 0.002};

  m_uniformHandle = m_engine->m_constantBufferManager->allocate(
      m_engine, sizeof(MBEUniforms),
      SirMetal::CONSTANT_BUFFER_FLAGS_BITS::CONSTANT_BUFFER_FLAG_BUFFERED);

  m_mesh = m_engine->m_meshManager->loadMesh("lucy.obj");

  id<MTLDevice> device = m_engine->m_renderingContext->getDevice();

  SirMetal::AllocTextureRequest requestDepth{
      static_cast<uint32_t>(1280 * 2),
      static_cast<uint32_t>(720 * 2),
      1,
      MTLTextureType2D,
      MTLPixelFormatDepth32Float_Stencil8,
      MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead,
      MTLStorageModePrivate,
      1,
      "depthTexture"};
  m_depthHandle = m_engine->m_textureManager->allocate(device, requestDepth);

  m_engine->m_shaderManager->loadShader("Shaders.metal");
}

void GraphicsLayer::onDetach() {}

template <class T> constexpr void hash_combine(std::size_t &seed, const T &v) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
std::size_t computePSOHash(const SirMetal::graphics::DrawTracker &tracker,
                           const SirMetal::Material &material) {
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

PSOCache getPSO(SirMetal::EngineContext *context,
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

  PSOCache cache{nil, nil};

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

  NSError *error = NULL;
  cache.color = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor
                                                       error:&error];
  m_psoCache[hash] = cache;
  return cache;
}

void GraphicsLayer::updateUniformsForView(float screenWidth,
                                          float screenHeight) {

  const matrix_float4x4 modelMatrix =
      matrix_float4x4_translation(vector_float3{0, 0, 0});
  MBEUniforms uniforms;
  uniforms.modelViewProjectionMatrix =
      matrix_multiply(m_camera.VP, modelMatrix);
  SirMetal::Input *input = m_engine->m_inputManager;
  m_cameraController.update(m_camConfig, input);
  uniforms.modelViewProjectionMatrix =
      matrix_multiply(m_camera.VP, modelMatrix);

  m_cameraController.updateProjection(screenWidth, screenHeight);
  m_engine->m_constantBufferManager->update(m_engine, m_uniformHandle,
                                            &uniforms);
}

void GraphicsLayer::onUpdate() {

  // NOTE: very temp ESC handy to close the game
  if (m_engine->m_inputManager->isKeyDown(SDL_SCANCODE_ESCAPE)) {
    SDL_Event sdlevent{};
    sdlevent.type = SDL_QUIT;
    SDL_PushEvent(&sdlevent);
  }

  CAMetalLayer *swapchain = m_engine->m_renderingContext->getSwapchain();
  id<MTLCommandQueue> queue = m_engine->m_renderingContext->getQueue();

  @autoreleasepool {
    id<CAMetalDrawable> surface = [swapchain nextDrawable];
    id<MTLTexture> texture = surface.texture;

    if (!surface) {
      return;
    }

    float w = texture.width;
    float h = texture.height;
    updateUniformsForView(w, h);

    SirMetal::graphics::DrawTracker tracker{};

    id<MTLDevice> device = m_engine->m_renderingContext->getDevice();
    tracker.renderTargets[0] = texture;
    tracker.depthTarget =
        m_engine->m_textureManager->getNativeFromHandle(m_depthHandle);
    PSOCache cache = getPSO(m_engine, tracker, SirMetal::Material{"Shaders",false});

    MTLRenderPassDescriptor *passDescriptor =
        [MTLRenderPassDescriptor renderPassDescriptor];
    passDescriptor.colorAttachments[0].texture = texture;
    passDescriptor.colorAttachments[0].clearColor =
        MTLClearColorMake(0.85, 0.85, 0.85, 1);
    passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;

    MTLRenderPassDepthAttachmentDescriptor *depthAttachment =
        passDescriptor.depthAttachment;
    depthAttachment.texture =
        m_engine->m_textureManager->getNativeFromHandle(m_depthHandle);
    depthAttachment.clearDepth = 1.0;
    depthAttachment.storeAction = MTLStoreActionDontCare;
    depthAttachment.loadAction = MTLLoadActionClear;

    id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];

    id<MTLRenderCommandEncoder> commandEncoder =
        [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];

    [commandEncoder setRenderPipelineState:cache.color];
    [commandEncoder setDepthStencilState:cache.depth];
    [commandEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
    [commandEncoder setCullMode:MTLCullModeBack];

    SirMetal::BindInfo info = m_engine->m_constantBufferManager->getBindInfo(
        m_engine, m_uniformHandle);
    [commandEncoder setVertexBuffer:info.buffer offset:info.offset atIndex:1];

    const SirMetal::MeshData *meshData =
        m_engine->m_meshManager->getMeshData(m_mesh);

    [commandEncoder setVertexBuffer:meshData->vertexBuffer offset:0 atIndex:0];
    [commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                               indexCount:meshData->primitivesCount
                                indexType:MTLIndexTypeUInt32
                              indexBuffer:meshData->indexBuffer
                        indexBufferOffset:0];
    [commandEncoder endEncoding];

    [commandBuffer presentDrawable:surface];
    [commandBuffer commit];
  }
}

bool GraphicsLayer::onEvent(SirMetal::Event &) {
  // TODO start processing events the game cares about, like
  // etc...
  return false;
}

void GraphicsLayer::clear() {}
} // namespace Sandbox

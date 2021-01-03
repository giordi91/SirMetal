#include "graphicsLayer.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <SirMetal/core/mathUtils.h>

#include "SirMetal/application/window.h"
#include "SirMetal/core/input.h"
#include "SirMetal/engine.h"
#include "SirMetal/graphics/PSOGenerator.h"
#include "SirMetal/graphics/constantBufferManager.h"
#include "SirMetal/graphics/debug/imgui/imgui.h"
#include "SirMetal/graphics/debug/imguiRenderer.h"
#include "SirMetal/graphics/materialManager.h"
#include "SirMetal/graphics/renderingContext.h"
#include "SirMetal/resources/meshes/meshManager.h"
#include "SirMetal/resources/shaderManager.h"
#include <SirMetal/resources/textureManager.h>


constexpr int kMaxInflightBuffers = 3;

namespace Sandbox {
void GraphicsLayer::onAttach(SirMetal::EngineContext *context) {

  m_engine = context;
  m_gpuAllocator.initialize(m_engine->m_renderingContext->getDevice(),
                            m_engine->m_renderingContext->getQueue());

  // initializing the camera to the identity
  m_camera.viewMatrix = matrix_float4x4_translation(vector_float3{0, 0, 0});
  m_camera.fov = M_PI / 4;
  m_camera.nearPlane = 0.01;
  m_camera.farPlane = 60;
  m_cameraController.setCamera(&m_camera);
  m_cameraController.setPosition(0, 1, 12);
  m_camConfig = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.2, 0.008};

  //allocating necessary uniform buffers
  m_camUniformHandle = m_engine->m_constantBufferManager->allocate(
          m_engine, sizeof(SirMetal::Camera),
          SirMetal::CONSTANT_BUFFER_FLAGS_BITS::CONSTANT_BUFFER_FLAG_BUFFERED);
  m_lightHandle = m_engine->m_constantBufferManager->allocate(
          m_engine, sizeof(DirLight),
          SirMetal::CONSTANT_BUFFER_FLAGS_BITS::CONSTANT_BUFFER_FLAG_NONE);

  updateLightData();

  //loading gltf data, which returns us an asset, with all the meshes, materials etc.
  //currently very basic and I will add more and more functionality on a need basis
  const std::string base = m_engine->m_config.m_dataSourcePath;
  const std::string baseSample = base + "/04_gltf_args";
  // let us load the gltf file

  struct SirMetal::GLTFLoadOptions options;
  options.flags= SirMetal::GLTFLoadFlags::GLTF_LOAD_FLAGS_FLATTEN_HIERARCHY;
  SirMetal::loadGLTF(m_engine, (baseSample + +"/test.glb").c_str(), m_asset, options);

  m_shaderHandle =
          m_engine->m_shaderManager->loadShader((baseSample + "/Shaders.metal").c_str());

  // now we process the args buffer, we currently use two argument buffers
  // one is used in the vertex shader to fetch the mesh data
  // the second one is used in the fragment shader to fetch the material data (for now a simple
  // texture + sampler and tint color
  id<MTLDevice> device = m_engine->m_renderingContext->getDevice();
  //arguments encoders are created from a shader function targeting a specific buffer
  //here we create one for the frag and one for the vert shader
  id<MTLFunction> fn = m_engine->m_shaderManager->getVertexFunction(m_shaderHandle);
  id<MTLArgumentEncoder> argumentEncoder = [fn newArgumentEncoderWithBufferIndex:0];
  id<MTLFunction> fnFrag = m_engine->m_shaderManager->getFragmentFunction(m_shaderHandle);
  id<MTLArgumentEncoder> argumentEncoderFrag =
          [fnFrag newArgumentEncoderWithBufferIndex:0];

  //the way argument buffer works is that the encoder  writes one element only
  //if you have an array of them you simply re-set the buffer by shifting the offset
  int meshesCount = m_asset.models.size();
  int buffInstanceSize = argumentEncoder.encodedLength;
  int buffInstanceSizeFrag = argumentEncoderFrag.encodedLength;
  //we allocate enough memory in the buffer to store the full data for all the meshes
  m_argBuffer = [device newBufferWithLength:buffInstanceSize * meshesCount options:0];
  m_argBufferFrag =
          [device newBufferWithLength:buffInstanceSizeFrag * meshesCount options:0];

  //create a single sampler that we will set on every material, technically this
  //can come from a gltf aswell
  MTLSamplerDescriptor *samplerDesc = [MTLSamplerDescriptor new];
  samplerDesc.minFilter = MTLSamplerMinMagFilterLinear;
  samplerDesc.magFilter = MTLSamplerMinMagFilterLinear;
  samplerDesc.mipFilter = MTLSamplerMipFilterNotMipmapped;
  samplerDesc.normalizedCoordinates = YES;
  samplerDesc.supportArgumentBuffers = YES;

  sampler = [device newSamplerStateWithDescriptor:samplerDesc];


  for (int i = 0; i < meshesCount; ++i) {

    //first we set the buffer offsetting by the size and the id
    [argumentEncoder setArgumentBuffer:m_argBuffer offset:i * buffInstanceSize];
    const auto *meshData = m_engine->m_meshManager->getMeshData(m_asset.models[i].mesh);
    //next we set all the buffers, to note that there is a call to set multiple
    //buffers in one go to make it even more efficient, might be worth changing the
    //way I store ranges to have SOA layout and re-use the arrays to set them in bulk
    [argumentEncoder setBuffer:meshData->vertexBuffer
                        offset:meshData->ranges[0].m_offset
                       atIndex:0];
    [argumentEncoder setBuffer:meshData->vertexBuffer
                        offset:meshData->ranges[1].m_offset
                       atIndex:1];
    [argumentEncoder setBuffer:meshData->vertexBuffer
                        offset:meshData->ranges[2].m_offset
                       atIndex:2];
    [argumentEncoder setBuffer:meshData->vertexBuffer
                        offset:meshData->ranges[3].m_offset
                       atIndex:3];
    [argumentEncoder setBuffer:meshData->indexBuffer offset:0 atIndex:4];


    //next we do the same exact process but for the material
    const auto &material = m_asset.materials[i];
    [argumentEncoderFrag setArgumentBuffer:m_argBufferFrag
                                    offset:i * buffInstanceSizeFrag];
    id albedo = m_engine->m_textureManager->getNativeFromHandle(material.colorTexture);
    [argumentEncoderFrag setTexture:albedo atIndex:0];
    [argumentEncoderFrag setSamplerState:sampler atIndex:1];

    //the constant data works slightly differently from the rest, you receive back a memory
    //mapped pointer you can perform your copy to
    auto *ptr = [argumentEncoderFrag constantDataAtIndex:2];
    memcpy(ptr, &material.colorFactors, sizeof(float) * 4);
  }

  SirMetal::AllocTextureRequest requestDepth{m_engine->m_config.m_windowConfig.m_width,
                                             m_engine->m_config.m_windowConfig.m_height,
                                             1,
                                             MTLTextureType2D,
                                             MTLPixelFormatDepth32Float_Stencil8,
                                             MTLTextureUsageRenderTarget |
                                                     MTLTextureUsageShaderRead,
                                             MTLStorageModePrivate,
                                             1,
                                             "depthTexture"};
  m_depthHandle = m_engine->m_textureManager->allocate(device, requestDepth);

  SirMetal::graphics::initImgui(m_engine);
  frameBoundarySemaphore = dispatch_semaphore_create(kMaxInflightBuffers);

  // this is to flush the gpu, making sure resources are properly loaded
  m_engine->m_renderingContext->flush();

  MTLArgumentBuffersTier tier = [device argumentBuffersSupport];
  assert(tier == MTLArgumentBuffersTier2);
}

void GraphicsLayer::onDetach() {}

void GraphicsLayer::updateUniformsForView(float screenWidth, float screenHeight) {

  SirMetal::Input *input = m_engine->m_inputManager;

  if (!ImGui::GetIO().WantCaptureMouse) { m_cameraController.update(m_camConfig, input); }

  m_cameraController.updateProjection(screenWidth, screenHeight);
  m_engine->m_constantBufferManager->update(m_engine, m_camUniformHandle, &m_camera);
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

  // waiting for resource
  dispatch_semaphore_wait(frameBoundarySemaphore, DISPATCH_TIME_FOREVER);

  id<CAMetalDrawable> surface = [swapchain nextDrawable];
  id<MTLTexture> texture = surface.texture;

  float w = texture.width;
  float h = texture.height;
  updateUniformsForView(w, h);
  updateLightData();

  // render
  SirMetal::graphics::DrawTracker tracker{};
  tracker.renderTargets[0] = texture;
  tracker.depthTarget = m_engine->m_textureManager->getNativeFromHandle(m_depthHandle);

  SirMetal::PSOCache cache =
          SirMetal::getPSO(m_engine, tracker, SirMetal::Material{"Shaders", false});

  id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];

  //this could be created once and reused since does not change but this is just a sample so I am not overly worried
  MTLRenderPassDescriptor *passDescriptor =
          [MTLRenderPassDescriptor renderPassDescriptor];
  passDescriptor.colorAttachments[0].texture = texture;
  passDescriptor.colorAttachments[0].clearColor = {0.2, 0.2, 0.2, 1.0};
  passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
  passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;

  MTLRenderPassDepthAttachmentDescriptor *depthAttachment =
          passDescriptor.depthAttachment;
  depthAttachment.texture =
          m_engine->m_textureManager->getNativeFromHandle(m_depthHandle);
  depthAttachment.clearDepth = 1.0;
  depthAttachment.storeAction = MTLStoreActionDontCare;
  depthAttachment.loadAction = MTLLoadActionClear;

  id<MTLRenderCommandEncoder> commandEncoder =
          [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];

  [commandEncoder setRenderPipelineState:cache.color];
  [commandEncoder setDepthStencilState:cache.depth];
  [commandEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
  [commandEncoder setCullMode:MTLCullModeBack];

  //first we set the uniform buffers like the camera and the light
  SirMetal::BindInfo info =
          m_engine->m_constantBufferManager->getBindInfo(m_engine, m_camUniformHandle);
  [commandEncoder setVertexBuffer:info.buffer offset:info.offset atIndex:4];
  info = m_engine->m_constantBufferManager->getBindInfo(m_engine, m_lightHandle);
  [commandEncoder setFragmentBuffer:info.buffer offset:info.offset atIndex:5];

  //next, setting the arguments buffer, to note we set them once outside the loop
  [commandEncoder setVertexBuffer:m_argBuffer offset:0 atIndex:0];
  [commandEncoder setFragmentBuffer:m_argBufferFrag offset:0 atIndex:0];

  int counter = 0;
  for (auto &mesh : m_asset.models) {

    //NOTE: now technically, you need to notify the metal driver about what resources you need to use in the argument
    //buffer, this works particularly well with heaps! In doing so the driver is able to figure out what necessary
    //barriers to issue or what resources to make available. In our case  we all have resources we copy manually on
    //the GPU and flush afterwards. Data is static so will always be available. Whether or not is good practice
    //to still signal the driver I am unsure, but this seems to work well without.
    //[commandEncoder useResource: m_engine->m_textureManager->getNativeFromHandle(mesh.material.colorTexture) usage:MTLResourceUsageSample];

    //next we set two piece of data, one is the object transform, we leave it as a push value
    //since it could be dynamic, and we also set an index that we use to index in the argument buffer array
    auto *data = (void *) (&mesh.matrix);
    [commandEncoder setVertexBytes:data length:16 * 4 atIndex:5];
    [commandEncoder setVertexBytes:&counter length:4 atIndex:6];

    //we still need to access the the mesh to know how many triangles to render.
    const SirMetal::MeshData *meshData = m_engine->m_meshManager->getMeshData(mesh.mesh);
    [commandEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                       vertexStart:0
                       vertexCount:meshData->primitivesCount];
    counter++;
  }

  // ui
  SirMetal::graphics::imguiNewFrame(m_engine, passDescriptor);
  renderDebugWindow();
  SirMetal::graphics::imguiEndFrame(commandBuffer, commandEncoder);

  [commandEncoder endEncoding];

  [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer) {
    // GPU work is complete
    // Signal the semaphore to start the CPU work
    dispatch_semaphore_signal(frameBoundarySemaphore);
  }];
  [commandBuffer presentDrawable:surface];
  [commandBuffer commit];

  //}
}

bool GraphicsLayer::onEvent(SirMetal::Event &) {
  // TODO start processing events the game cares about, like
  // etc...
  return false;
}

void GraphicsLayer::clear() {
  m_engine->m_renderingContext->flush();
  SirMetal::graphics::shutdownImgui();
}
void GraphicsLayer::updateLightData() {
  simd_float3 view{0, 1, 1};
  view = simd_normalize(view);
  simd_float3 up{0, 1, 0};
  simd_float3 cross = simd_normalize(simd_cross(up, view));
  up = simd_normalize(simd_cross(view, cross));
  simd_float4 pos4{0, 15, 15, 1};

  simd_float4 view4{view.x, view.y, view.z, 0};
  simd_float4 up4{up.x, up.y, up.z, 0};
  simd_float4 cross4{cross.x, cross.y, cross.z, 0};
  light.V = {cross4, up4, view4, pos4};
  light.P = matrix_float4x4_perspective(1, M_PI / 2, 0.01f, 40.0f);
  // light.VInverse = simd_inverse(light.V);
  light.VP = simd_mul(light.P, simd_inverse(light.V));
  light.lightDir = view4;

  m_engine->m_constantBufferManager->update(m_engine, m_lightHandle, &light);
}
void GraphicsLayer::renderDebugWindow() {

  static bool p_open = false;
  if (m_engine->m_inputManager->isKeyReleased(SDL_SCANCODE_GRAVE)) { p_open = !p_open; }
  if (!p_open) { return; }
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
  if (ImGui::Begin("Debug", &p_open, 0)) { m_timingsWidget.render(m_engine); }
  ImGui::End();
}
}// namespace Sandbox

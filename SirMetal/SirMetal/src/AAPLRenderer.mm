#import <Metal/Metal.h>
#import "MetalKit/MetalKit.h"

#import "AAPLRenderer.h"
#include "SirMetalLib/engineContext.h"
#include "SirMetalLib/resources/shaderManager.h"
#include "SirMetalLib/resources/textureManager.h"
#import "SirMetalLib/MBEMathUtilities.h"
#import "vendors/imgui/imgui.h"
#import "vendors/imgui/imgui_impl_metal.h"
#import "SirMetalLib/resources/meshes/meshManager.h"
#import "imgui_impl_osx.h"
#import "imgui_internal.h"
#import "editorUI.h"
#import "SirMetalLib/core/logging/log.h"
#import "project.h"
#import "SirMetalLib/core/flags.h"
#import "SirMetalLib/graphics/materialManager.h"
#import "SirMetalLib/graphics/renderingContext.h"
#import "ImGuizmo.h"
#import "SirMetalLib/core/memory/gpu/GPUMemoryAllocator.h"
#import "SirMetalLib/graphics/constantBufferManager.h"
#import "SirMetalLib/graphics/PSOGenerator.h"
#import "SirMetalLib/graphics/techniques/selectionRendering.h"

static const NSInteger MBEInFlightBufferCount = 3;
const MTLIndexType MBEIndexType = MTLIndexTypeUInt32;

typedef struct {
  matrix_float4x4 modelViewProjectionMatrix;
} MBEUniforms;

static inline uint64_t AlignUp(uint64_t n, uint32_t alignment) {
  return ((n + alignment - 1) / alignment) * alignment;
}

static const uint32_t MBEBufferAlignment = 256;

//temporary until i figure out what to do with this
static SirMetal::Editor::EditorUI editorUI = SirMetal::Editor::EditorUI();
static SirMetal::Material m_opaqueMaterial;
static SirMetal::DrawTracker m_drawTracker;
static SirMetal::ConstantBufferHandle m_uniformHandle;
static SirMetal::Selection m_selection;

@interface AAPLRenderer ()
@property(strong) id<MTLTexture> depthTexture;
@property(strong) id<MTLTexture> depthTextureGUI;
@property(strong) id<MTLTexture> offScreenTexture;
@property(strong) dispatch_semaphore_t displaySemaphore;
@property(strong) dispatch_semaphore_t resizeSemaphore;
@property(assign) NSInteger bufferIndex;
@end

// Main class performing the rendering
@implementation AAPLRenderer {
  id<MTLDevice> _device;

  //// The command queue used to pass commands to the device.
  //id <MTLCommandQueue> _commandQueue;
}

- (void)logGPUInformation:(id<MTLDevice>)device {
  NSLog(@"Device name: %@", device.name);
  NSLog(@"\t\tis low power: %@", device.isLowPower ? @"YES" : @"NO");
  NSLog(@"\t\tis removable: %@", device.isRemovable ? @"YES" : @"NO");
  NSLog(@"\t\tsupport macOS GPU family 1: %@",
        [device supportsFeatureSet:MTLFeatureSet::MTLFeatureSet_macOS_GPUFamily1_v1] ? @"YES" : @"NO");
  NSLog(@"\t\tsupport macOS GPU family 2: %@",
        [device supportsFeatureSet:MTLFeatureSet::MTLFeatureSet_macOS_GPUFamily2_v1] ? @"YES" : @"NO");
}

- (nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)mtkView {
  self = [super init];
  if (self) {
    _device = mtkView.device;
    _displaySemaphore = dispatch_semaphore_create(MBEInFlightBufferCount);
    _resizeSemaphore = dispatch_semaphore_create(0);


    // Create the command queue
    _commandQueue = [_device newCommandQueue];

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplMetal_Init(_device);

    NSLog(@"Initializing Sir Metal: v0.0.1");
    [self logGPUInformation:_device];

    mtkView.paused = NO;

    self.screenWidth = mtkView.drawableSize.width;
    self.screenHeight = mtkView.drawableSize.height;

  }
  //set up temporary stuff
  m_opaqueMaterial.shaderName = "Shaders";
  m_opaqueMaterial.state.enabled = false;


  for (int i = 0; i < SirMetal::MAX_COLOR_ATTACHMENT; ++i) {
    m_drawTracker.renderTargets[i] = nil;
  }
  return self;
}

- (void)initGraphicsObjectsTemp {

  //create the pipeline
  [self makeBuffers];
  //TODO probably grab the start viewport size from the context
  [self createOffscreenTexture:256 :256];

  //view matrix
  //initializing the camera to the identity
  SirMetal::Camera &camera = SirMetal::CONTEXT->camera;
  SirMetal::EditorFPSCameraController &cameraController = SirMetal::CONTEXT->cameraController;
  camera.viewMatrix = matrix_float4x4_translation(vector_float3{0, 0, 0});
  camera.fov = static_cast<float>(M_PI / 4);
  camera.nearPlane = 1;
  camera.farPlane = 100;
  cameraController.setCamera(&camera);
  cameraController.setPosition(0, 0, 5);

  MTLTextureDescriptor *descriptor =
      [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float_Stencil8
                                                         width:self.screenWidth height:self.screenHeight mipmapped:NO];
  descriptor.storageMode = MTLStorageModePrivate;
  descriptor.usage = MTLTextureUsageRenderTarget;
  descriptor.pixelFormat = MTLPixelFormatDepth32Float_Stencil8;
  self.depthTextureGUI = [_device newTextureWithDescriptor:descriptor];
  self.depthTextureGUI.label = @"DepthStencilGUI";

  SirMetal::MeshHandle mesh = SirMetal::CONTEXT->managers.meshManager->getHandleFromName("lucy");
  SirMetal::MeshHandle cube = SirMetal::CONTEXT->managers.meshManager->getHandleFromName("cube");
  SirMetal::MeshHandle cone = SirMetal::CONTEXT->managers.meshManager->getHandleFromName("cone");
  SirMetal::CONTEXT->world.hierarchy.addToRoot(mesh);
  SirMetal::CONTEXT->world.hierarchy.addToRoot(cube);
  SirMetal::CONTEXT->world.hierarchy.addToRoot(cone);

  m_selection.initialize(_device);

}

- (void)makeBuffers {

  auto *cbManager = SirMetal::CONTEXT->managers.constantBufferManager;
  m_uniformHandle = cbManager->allocate(sizeof(MBEUniforms), SirMetal::CONSTANT_BUFFER_FLAG_BUFFERED);
}

- (void)updateUniformsForView:(float)screenWidth :(float)screenHeight {

  SirMetal::Camera &camera = SirMetal::CONTEXT->camera;
  SirMetal::EditorFPSCameraController &cameraController = SirMetal::CONTEXT->cameraController;

  auto handle = SirMetal::CONTEXT->managers.meshManager->getHandleFromName("lucy");
  auto *data = SirMetal::CONTEXT->managers.meshManager->getMeshData(handle);
  const matrix_float4x4 modelMatrix = data->modelMatrix;

  ImGuiIO &io = ImGui::GetIO();
  //we wish to update the camera aka move it only when we are in
  //viewport mode, if the viewport is in focus, or when we are not
  //in viewport mode, meaning fullscreen. if one of those two conditions
  //is true we update the camera.
  bool isViewport = (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) > 0;
  bool isViewportInFocus =
      isFlagSet(SirMetal::CONTEXT->flags.interaction, SirMetal::InteractionFlagsBits::InteractionViewportFocused);
  bool isManipulator =
      isFlagSet(SirMetal::CONTEXT->flags.interaction, SirMetal::InteractionFlagsBits::InteractionViewportGuizmo);

  bool shouldControlViewport = isViewport & isViewportInFocus & (!isManipulator);
  MBEUniforms uniforms;
  uniforms.modelViewProjectionMatrix = matrix_multiply(camera.VP, modelMatrix);
  if (shouldControlViewport | (!isViewport)) {
    SirMetal::Input &input = SirMetal::CONTEXT->input;

    const SirMetal::CameraManipulationConfig &camConfig = SirMetal::Editor::PROJECT->getSettings().m_cameraConfig;
    cameraController.update(camConfig, &input);
    uniforms.modelViewProjectionMatrix = matrix_multiply(camera.VP, modelMatrix);

  }
  cameraController.updateProjection(screenWidth, screenHeight);

  auto *cbManager = SirMetal::CONTEXT->managers.constantBufferManager;
  cbManager->update(m_uniformHandle, &uniforms);
}

- (void)createOffscreenTexture:(int)width :(int)height {

  SirMetal::TextureManager *textureManager = SirMetal::CONTEXT->managers.textureManager;

  SirMetal::AllocTextureRequest request{
      static_cast<uint32_t>(width), static_cast<uint32_t>(height),
      1, MTLTextureType2D, MTLPixelFormatBGRA8Unorm,
      MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead, MTLStorageModePrivate, 1, "offscreenTexture"
  };
  self.viewportHandle = textureManager->allocate(_device, request);
  self.offScreenTexture = textureManager->getNativeFromHandle(self.viewportHandle);
  SirMetal::CONTEXT->viewportTexture = (__bridge void *) self.offScreenTexture;

  SirMetal::AllocTextureRequest requestDepth{
      static_cast<uint32_t>(width), static_cast<uint32_t>(height),
      1, MTLTextureType2D, MTLPixelFormatDepth32Float_Stencil8,
      MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead, MTLStorageModePrivate, 1, "depthTexture"
  };
  self.depthHandle = textureManager->allocate(_device, requestDepth);
  self.depthTexture = textureManager->getNativeFromHandle(self.depthHandle);
}

- (void)renderUI:(MTKView *)view :(MTLRenderPassDescriptor *)passDescriptor :(id<MTLCommandBuffer>)commandBuffer
    :(id<MTLRenderCommandEncoder>)renderPass {
  // Start the Dear ImGui frame
  ImGui_ImplOSX_NewFrame(view);
  ImGui_ImplMetal_NewFrame(passDescriptor);
  ImGui::NewFrame();



  //[self showImguiContent];
  editorUI.show(SirMetal::CONTEXT->screenWidth, SirMetal::CONTEXT->screenHeight);
  // Rendering
  ImGui::Render();
  ImDrawData *drawData = ImGui::GetDrawData();
  ImGui_ImplMetal_RenderDrawData(drawData, commandBuffer, renderPass);
}

/// Called whenever the view needs to render a frame.
- (void)drawInMTKView:(nonnull MTKView *)view {

  static int frame = 0;
  frame++;
  //dispatch_semaphore_wait(self.displaySemaphore, DISPATCH_TIME_FOREVER);
  ImVec2 viewportSize = editorUI.getViewportSize();
  bool viewportChanged = SirMetal::isFlagSet(SirMetal::CONTEXT->flags.viewEvents,
                                             SirMetal::ViewEventsFlagsBits::ViewEventsViewportSizeChanged);
  if (viewportChanged) {

    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer) {
      dispatch_semaphore_signal(self.resizeSemaphore);
    }];
    [commandBuffer commit];
    dispatch_semaphore_wait(self.resizeSemaphore, DISPATCH_TIME_FOREVER);

    SirMetal::TextureManager *texManager = SirMetal::CONTEXT->managers.textureManager;
    bool viewportResult = texManager->resizeTexture(_device, self.viewportHandle, viewportSize.x, viewportSize.y);
    bool depthResult = texManager->resizeTexture(_device, self.depthHandle, viewportSize.x, viewportSize.y);
    if ((!viewportResult) | (!depthResult)) {
      SIR_CORE_FATAL("Could not resize viewport color or depth buffer");
    } else {
      //if we got no error we extract the new texture so that can be used
      self.offScreenTexture = texManager->getNativeFromHandle(self.viewportHandle);
      self.depthTexture = texManager->getNativeFromHandle(self.depthHandle);
      SirMetal::CONTEXT->viewportTexture = (__bridge void *) self.offScreenTexture;
      SirMetal::CONTEXT->cameraController.updateProjection(viewportSize.x, viewportSize.y);

    }
    SirMetal::setFlagBitfield(SirMetal::CONTEXT->flags.viewEvents,
                              SirMetal::ViewEventsFlagsBits::ViewEventsViewportSizeChanged,
                              false);
  }

  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize.x = static_cast<float>(view.bounds.size.width);
  io.DisplaySize.y = static_cast<float>(view.bounds.size.height);

  CGFloat framebufferScale = view.window.screen.backingScaleFactor ?: NSScreen.mainScreen.backingScaleFactor;
  io.DisplayFramebufferScale = ImVec2(framebufferScale, framebufferScale);
  io.DeltaTime = 1 / float(view.preferredFramesPerSecond ?: 60);

  view.clearColor = MTLClearColorMake(0.95, 0.95, 0.95, 1);

  float w = view.drawableSize.width;
  float h = view.drawableSize.height;

  bool isViewport = (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) > 0;
  [self updateUniformsForView:(isViewport ? viewportSize.x : w) :(isViewport ? viewportSize.y : h)];

  id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
  MTLRenderPassDescriptor *passDescriptor = [view currentRenderPassDescriptor];
  //passDescriptor.depthAttachment

  float wToUse = isViewport ? self.offScreenTexture.width : view.currentDrawable.texture.width;
  float hToUse = isViewport ? self.offScreenTexture.height : view.currentDrawable.texture.height;

  passDescriptor.renderTargetWidth = wToUse;
  passDescriptor.renderTargetHeight = hToUse;
  MTLRenderPassColorAttachmentDescriptor *colorAttachment = passDescriptor.colorAttachments[0];
  colorAttachment.texture = isViewport ? self.offScreenTexture : view.currentDrawable.texture;
  colorAttachment.clearColor = MTLClearColorMake(0.8, 0.8, 0.8, 1.0);
  colorAttachment.storeAction = MTLStoreActionStore;
  colorAttachment.loadAction = MTLLoadActionClear;

  MTLRenderPassDepthAttachmentDescriptor *depthAttachment = passDescriptor.depthAttachment;
  depthAttachment.texture = isViewport ? self.depthTexture : self.depthTextureGUI;
  depthAttachment.clearDepth = 1.0;
  depthAttachment.storeAction = MTLStoreActionDontCare;
  depthAttachment.loadAction = MTLLoadActionClear;

  m_drawTracker.renderTargets[0] = isViewport ? self.offScreenTexture : view.currentDrawable.texture;
  m_drawTracker.depthTarget = isViewport ? self.depthTexture : self.depthTextureGUI;
  SirMetal::PSOCache cache = getPSO(_device, m_drawTracker, m_opaqueMaterial);

  id<MTLRenderCommandEncoder> renderPass = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];
  [renderPass setRenderPipelineState:cache.color];
  [renderPass setDepthStencilState:cache.depth];
  [renderPass setFrontFacingWinding:MTLWindingCounterClockwise];
  [renderPass setCullMode:MTLCullModeBack];

  auto *cbManager = SirMetal::CONTEXT->managers.constantBufferManager;
  SirMetal::BindInfo bindInfo = cbManager->getBindInfo(m_uniformHandle);

  [renderPass setVertexBuffer:bindInfo.buffer offset:bindInfo.offset atIndex:1];

  auto nodes = SirMetal::CONTEXT->world.hierarchy.getNodes();
  size_t nodeSize = nodes.size();
  //skipping the root
  for (size_t i = 1; i < nodeSize; ++i) {
    const SirMetal::DenseTreeNode &node = nodes[i];
    //SirMetal::MeshHandle mesh = SirMetal::CONTEXT->managers.meshManager->getHandleFromName("lucy");
    const SirMetal::MeshData
        *meshData = SirMetal::CONTEXT->managers.meshManager->getMeshData(SirMetal::MeshHandle{node.id});

    [renderPass setVertexBuffer:meshData->vertexBuffer offset:0 atIndex:0];
    [renderPass drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                           indexCount:meshData->primitivesCount
                            indexType:MBEIndexType
                          indexBuffer:meshData->indexBuffer
                    indexBufferOffset:0];

  }

  [renderPass endEncoding];

  if (SirMetal::CONTEXT->world.hierarchy.getSelectedId() != -1) {
    //[self renderSelection:wToUse :hToUse :commandBuffer :&m_drawTracker];
    m_selection.render(_device, commandBuffer, wToUse, hToUse, m_uniformHandle, self.offScreenTexture);
  }

  if (isViewport) {

    MTLRenderPassDescriptor *uiPassDescriptor = [[MTLRenderPassDescriptor alloc] init];
    MTLRenderPassColorAttachmentDescriptor *uiColorAttachment = uiPassDescriptor.colorAttachments[0];
    uiColorAttachment.texture = view.currentDrawable.texture;
    uiColorAttachment.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    uiColorAttachment.storeAction = MTLStoreActionStore;
    uiColorAttachment.loadAction = MTLLoadActionClear;

    MTLRenderPassDepthAttachmentDescriptor *uiDepthAttachment = uiPassDescriptor.depthAttachment;
    uiDepthAttachment.texture = self.depthTextureGUI;
    uiDepthAttachment.clearDepth = 1.0;
    uiDepthAttachment.storeAction = MTLStoreActionDontCare;
    uiDepthAttachment.loadAction = MTLLoadActionClear;

    uiPassDescriptor.renderTargetWidth = w;
    uiPassDescriptor.renderTargetHeight = h;

    id<MTLRenderCommandEncoder> uiPass = [commandBuffer renderCommandEncoderWithDescriptor:uiPassDescriptor];

    [self renderUI:view :passDescriptor :commandBuffer :uiPass];

    [uiPass endEncoding];

  } else {
    MTLRenderPassDescriptor *uiPassDescriptor = [[MTLRenderPassDescriptor alloc] init];
    MTLRenderPassColorAttachmentDescriptor *uiColorAttachment = uiPassDescriptor.colorAttachments[0];
    uiColorAttachment.texture = view.currentDrawable.texture;
    uiColorAttachment.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    uiColorAttachment.storeAction = MTLStoreActionStore;
    uiColorAttachment.loadAction = MTLLoadActionLoad;

    MTLRenderPassDepthAttachmentDescriptor *uiDepthAttachment = uiPassDescriptor.depthAttachment;
    //uiDepthAttachment.texture = self.depthTextureGUI;
    uiDepthAttachment.texture = nil;
    uiDepthAttachment.clearDepth = 1.0;
    uiDepthAttachment.storeAction = MTLStoreActionDontCare;
    uiDepthAttachment.loadAction = MTLLoadActionClear;

    uiPassDescriptor.renderTargetWidth = w;
    uiPassDescriptor.renderTargetHeight = h;

    id<MTLRenderCommandEncoder> uiPass = [commandBuffer renderCommandEncoderWithDescriptor:uiPassDescriptor];

    [self renderUI2:view :passDescriptor :commandBuffer :uiPass];

    [uiPass endEncoding];

  }
  SirMetal::endFrame(SirMetal::CONTEXT);

  [commandBuffer presentDrawable:view.currentDrawable];

  //[commandBuffer addCompletedHandler:^(id <MTLCommandBuffer> commandBuffer) {
  //    self.bufferIndex = (self.bufferIndex + 1) % MBEInFlightBufferCount;
  //    dispatch_semaphore_signal(self.displaySemaphore);
  //}];

  [commandBuffer commit];

}

- (void)renderUI2:(MTKView *)view :(MTLRenderPassDescriptor *)descriptor :(id<MTLCommandBuffer>)buffer :(id<MTLRenderCommandEncoder>)pass {

  // Start the Dear ImGui frame
  ImGui_ImplOSX_NewFrame(view);
  ImGui_ImplMetal_NewFrame(descriptor);
  ImGui::NewFrame();
  ImGuizmo::BeginFrame();

  //[self showImguiContent];
  editorUI.show2(SirMetal::CONTEXT->screenWidth, SirMetal::CONTEXT->screenHeight);
  // Rendering
  ImGui::Render();
  ImDrawData *drawData = ImGui::GetDrawData();
  ImGui_ImplMetal_RenderDrawData(drawData, buffer, pass);
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {

  //here we divide by the scale factor so our engine will work with the
  //real sizes if needed and not scaled up views
  CGFloat scaling = view.window.screen.backingScaleFactor;
  SirMetal::CONTEXT->screenWidth = static_cast<uint32_t>(size.width / scaling);
  SirMetal::CONTEXT->screenHeight = static_cast<uint32_t>(size.height / scaling);
}

@end


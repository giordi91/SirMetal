#import <Metal/Metal.h>
#import "MetalKit/MetalKit.h"

#import "AAPLRenderer.h"
#include "engineContext.h"
#include "shaderManager.h"
#include "resources/textureManager.h"
#import "MBEMathUtilities.h"
#import "vendors/imgui/imgui.h"
#import "vendors/imgui/imgui_impl_metal.h"
#import "resources/meshes/meshManager.h"
#import "imgui_impl_osx.h"
#import "imgui_internal.h"
#import "editorUI.h"
#import "log.h"
#import "project.h"
#import "core/flags.h"
#import "materialManager.h"
#import "renderingContext.h"

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
struct PSOCache {
    id <MTLRenderPipelineState> color = nil;
    id <MTLDepthStencilState> depth = nil;
};
static SirMetal::Editor::EditorUI editorUI = SirMetal::Editor::EditorUI();
static std::unordered_map<std::size_t, PSOCache> m_psoCache;
static SirMetal::Material m_opaqueMaterial;
static SirMetal::Material m_jumpFoodMaterial;
static SirMetal::Material m_jumpFloodMaskMaterial;
static SirMetal::Material m_jumpFloodInitMaterial;
static SirMetal::Material m_outlineMaterial;
static SirMetal::DrawTracker m_drawTracker;
static const uint MAX_COLOR_ATTACHMENT = 8;

@interface AAPLRenderer ()
@property(strong) id <MTLRenderPipelineState> jumpMaskPipelineState;
@property(strong) id <MTLRenderPipelineState> jumpInitPipelineState;
@property(strong) id <MTLRenderPipelineState> jumpOutlinePipelineState;
@property(strong) id <MTLRenderPipelineState> jumpPipelineState;
@property(strong) id <MTLTexture> depthTexture;
@property(strong) id <MTLTexture> jumpTexture;
@property(strong) id <MTLTexture> jumpTexture2;
@property(strong) id <MTLTexture> jumpMaskTexture;
@property(strong) id <MTLTexture> depthTextureGUI;
@property(strong) id <MTLTexture> offScreenTexture;
@property(nonatomic) SirMetal::TextureHandle jumpHandle;
@property(nonatomic) SirMetal::TextureHandle jumpHandle2;
@property(nonatomic) SirMetal::TextureHandle jumpMaskHandle;
@property(strong) id <MTLBuffer> uniformBuffer;
@property(strong) id <MTLBuffer> floodUniform;
@property(strong) dispatch_semaphore_t displaySemaphore;
@property(strong) dispatch_semaphore_t resizeSemaphore;
@property(assign) NSInteger bufferIndex;
@end

void updateVoidIndices(int w, int h, id <MTLBuffer> buffer) {
    std::vector<int> indices;
    indices.resize(256 / 4 * 16);
    int N = pow(2, 16);
    for (int i = 0; i < 16; ++i) {
        int offset = pow(2, (log2(N) - i - 1));
        int id = i * 64;
        indices[id] = offset;
        indices[id + 1] = w;
        indices[id + 2] = h;
    }
    memcpy((char *) ([buffer contents]), indices.data(), sizeof(int) * indices.size());

}

// Main class performing the rendering
@implementation AAPLRenderer {
    id <MTLDevice> _device;

    // The command queue used to pass commands to the device.
    id <MTLCommandQueue> _commandQueue;
}

- (void)logGPUInformation:(id <MTLDevice>)device {
    NSLog(@"Device name: %@", device.name);
    NSLog(@"\t\tis low power: %@", device.isLowPower ? @"YES" : @"NO");
    NSLog(@"\t\tis removable: %@", device.isRemovable ? @"YES" : @"NO");
    NSLog(@"\t\tsupport macOS GPU family 1: %@", [device supportsFeatureSet:MTLFeatureSet::MTLFeatureSet_macOS_GPUFamily1_v1] ? @"YES" : @"NO");
    NSLog(@"\t\tsupport macOS GPU family 2: %@", [device supportsFeatureSet:MTLFeatureSet::MTLFeatureSet_macOS_GPUFamily2_v1] ? @"YES" : @"NO");
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

    m_jumpFloodInitMaterial.shaderName = "jumpInit";
    m_jumpFloodInitMaterial.state.enabled = false;
    m_jumpFloodMaskMaterial.shaderName = "jumpMask";
    m_jumpFloodMaskMaterial.state.enabled = false;
    for (int i = 0; i < MAX_COLOR_ATTACHMENT; ++i) {
        m_drawTracker.renderTargets[i] = nil;
    }

    return self;
}

- (void)initGraphicsObjectsTemp {

    //create the pipeline
    [self makePipeline];
    [self makeBuffers];
    //TODO probably grab the start viewport size from the context
    [self createOffscreenTexture:256 :256];

    //view matrix
    //initializing the camera to the identity
    SirMetal::Camera &camera = SirMetal::CONTEXT->camera;
    SirMetal::EditorFPSCameraController &cameraController = SirMetal::CONTEXT->cameraController;
    camera.viewMatrix = matrix_float4x4_translation(vector_float3{0, 0, 0});
    camera.fov = M_PI / 4;
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

}

- (void)makePipeline {

    SirMetal::ShaderManager *sManager = SirMetal::CONTEXT->managers.shaderManager;

    NSError *error = NULL;
    //JUMP FLOOD MASK
    SirMetal::LibraryHandle lh;
    lh = SirMetal::CONTEXT->managers.shaderManager->getHandleFromName("jumpMask");
    id <MTLLibrary> rawLib = SirMetal::CONTEXT->managers.shaderManager->getLibraryFromHandle(lh);

    MTLRenderPipelineDescriptor *pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.vertexFunction = [rawLib newFunctionWithName:@"vertex_project"];
    pipelineDescriptor.fragmentFunction = [rawLib newFunctionWithName:@"fragment_flatcolor"];
    pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatR8Unorm;

    self.jumpMaskPipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];

    if (!self.jumpMaskPipelineState) {
        NSLog(@"Error occurred when creating jump mask render pipeline state: %@", error);
    }

    //jump flo0d init
    lh = SirMetal::CONTEXT->managers.shaderManager->getHandleFromName("jumpInit");
    rawLib = SirMetal::CONTEXT->managers.shaderManager->getLibraryFromHandle(lh);

    pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.vertexFunction = [rawLib newFunctionWithName:@"vertex_project"];
    pipelineDescriptor.fragmentFunction = [rawLib newFunctionWithName:@"fragment_flatcolor"];
    pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatRG32Float;

    self.jumpInitPipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];

    if (!self.jumpInitPipelineState) {
        NSLog(@"Error occurred when creating jump init render pipeline state: %@", error);
    }
    //jump flo0d
    lh = SirMetal::CONTEXT->managers.shaderManager->getHandleFromName("jumpFlood");
    rawLib = SirMetal::CONTEXT->managers.shaderManager->getLibraryFromHandle(lh);

    pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.vertexFunction = [rawLib newFunctionWithName:@"vertex_project"];
    pipelineDescriptor.fragmentFunction = [rawLib newFunctionWithName:@"fragment_flatcolor"];
    pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatRG32Float;

    self.jumpPipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];

    if (!self.jumpPipelineState) {
        NSLog(@"Error occurred when creating jump render pipeline state: %@", error);
    }

    //jump outline
    lh = SirMetal::CONTEXT->managers.shaderManager->getHandleFromName("jumpOutline");
    rawLib = SirMetal::CONTEXT->managers.shaderManager->getLibraryFromHandle(lh);

    pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.vertexFunction = [rawLib newFunctionWithName:@"vertex_project"];
    pipelineDescriptor.fragmentFunction = [rawLib newFunctionWithName:@"fragment_flatcolor"];
    pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    pipelineDescriptor.colorAttachments[0].blendingEnabled = YES;
    pipelineDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    pipelineDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

    self.jumpOutlinePipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];

    if (!self.jumpOutlinePipelineState) {
        NSLog(@"Error occurred when creating jump outline render pipeline state: %@", error);
    }
}

- (void)makeBuffers {
    _uniformBuffer = [_device newBufferWithLength:AlignUp(sizeof(MBEUniforms), MBEBufferAlignment) * MBEInFlightBufferCount
                                          options:MTLResourceOptionCPUCacheModeDefault];
    [_uniformBuffer setLabel:@"Uniforms"];

    //allocating 16 values, then just moving the offset when binding, now
    //i am not sure what is the minimum stride so I am going for a 32*4 bits
    int sizeFlood = 256 * 16;
    _floodUniform = [_device newBufferWithLength:AlignUp(sizeFlood, MBEBufferAlignment) * MBEInFlightBufferCount
                                         options:MTLResourceOptionCPUCacheModeDefault];
    [_uniformBuffer setLabel:@"floodUniforms"];

    int w = 256;
    int h = 256;
    updateVoidIndices(w, h, self.floodUniform);
}


- (void)updateUniformsForView:(float)screenWidth :(float)screenHeight {

    SirMetal::Camera &camera = SirMetal::CONTEXT->camera;
    SirMetal::EditorFPSCameraController &cameraController = SirMetal::CONTEXT->cameraController;

    const matrix_float4x4 modelMatrix = matrix_float4x4_translation(vector_float3{0, 0, 0});

    ImGuiIO &io = ImGui::GetIO();
    //we wish to update the camera aka move it only when we are in
    //viewport mode, if the viewport is in focus, or when we are not
    //in viewport mode, meaning fullscreen. if one of those two conditions
    //is true we update the camera.
    bool isViewport = (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) > 0;
    bool isViewportInFocus = isFlagSet(SirMetal::CONTEXT->flags.interaction, SirMetal::InteractionFlagsBits::InteractionViewportFocused);

    bool shouldControlViewport = isViewport & isViewportInFocus;
    MBEUniforms uniforms;
    uniforms.modelViewProjectionMatrix = matrix_multiply(camera.VP, modelMatrix);
    if (shouldControlViewport | (!isViewport)) {
        SirMetal::Input &input = SirMetal::CONTEXT->input;
        cameraController.update(&input);
        uniforms.modelViewProjectionMatrix = matrix_multiply(camera.VP, modelMatrix);

    }
    cameraController.updateProjection(screenWidth, screenHeight);
    const NSUInteger uniformBufferOffset = AlignUp(sizeof(MBEUniforms), MBEBufferAlignment) * self.bufferIndex;
    memcpy((char *) ([self.uniformBuffer contents]) + uniformBufferOffset, &uniforms, sizeof(uniforms));
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
    SirMetal::AllocTextureRequest jumpRequest{
            static_cast<uint32_t>(width), static_cast<uint32_t>(height),
            1, MTLTextureType2D, MTLPixelFormatRG32Float,
            MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead, MTLStorageModePrivate, 1, "jumpFloodTexture"
    };
    self.jumpHandle = textureManager->allocate(_device, jumpRequest);
    self.jumpTexture = textureManager->getNativeFromHandle(self.jumpHandle);
    //second texture used to do ping pong for selection
    jumpRequest.name = "jumpRequest2";
    self.jumpHandle2 = textureManager->allocate(_device, jumpRequest);
    self.jumpTexture2 = textureManager->getNativeFromHandle(self.jumpHandle2);

    SirMetal::AllocTextureRequest jumpMaskRequest{
            static_cast<uint32_t>(width), static_cast<uint32_t>(height),
            1, MTLTextureType2D, MTLPixelFormatR8Unorm,
            MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead, MTLStorageModePrivate, 1, "jumpFloodMaskTexture"
    };
    self.jumpMaskHandle = textureManager->allocate(_device, jumpMaskRequest);
    self.jumpMaskTexture = textureManager->getNativeFromHandle(self.jumpMaskHandle);
}

- (void)renderUI:(MTKView *)view :(MTLRenderPassDescriptor *)passDescriptor :(id <MTLCommandBuffer>)commandBuffer
        :(id <MTLRenderCommandEncoder>)renderPass {
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

template<class T>
constexpr void hash_combine(std::size_t &seed, const T &v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}


std::size_t computePSOHash(const SirMetal::DrawTracker &tracker, const SirMetal::Material &material) {
    std::size_t toReturn = 0;

    for (int i = 0; i < MAX_COLOR_ATTACHMENT; ++i) {
        if (tracker.renderTargets[i] != nil) {
            id <MTLTexture> tex = tracker.renderTargets[i];
            hash_combine(toReturn, static_cast<int>(tex.pixelFormat));
        }
    }
    if (tracker.depthTarget != nil) {
        id <MTLTexture> tex = tracker.depthTarget;
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


PSOCache getPSO(id <MTLDevice> device, const SirMetal::DrawTracker &tracker, const SirMetal::Material &material) {

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
        id <MTLTexture> texture = tracker.renderTargets[i];
        pipelineDescriptor.colorAttachments[i].pixelFormat = texture.pixelFormat;
    }
    PSOCache cache{nil, nil};

    if (tracker.depthTarget != nil) {
        //need to do depth stuff here
        id <MTLTexture> texture = tracker.depthTarget;
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


/// Called whenever the view needs to render a frame.
- (void)drawInMTKView:(nonnull MTKView *)view {

    static int frame = 0;
    frame++;
    dispatch_semaphore_wait(self.displaySemaphore, DISPATCH_TIME_FOREVER);
    ImVec2 viewportSize = editorUI.getViewportSize();
    bool viewportChanged = SirMetal::isFlagSet(SirMetal::CONTEXT->flags.viewEvents, SirMetal::ViewEventsFlagsBits::ViewEventsViewportSizeChanged);
    if (viewportChanged) {

        id <MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
        [commandBuffer addCompletedHandler:^(id <MTLCommandBuffer> commandBuffer) {
            dispatch_semaphore_signal(self.resizeSemaphore);
        }];
        [commandBuffer commit];
        dispatch_semaphore_wait(self.resizeSemaphore, DISPATCH_TIME_FOREVER);

        SirMetal::TextureManager *texManager = SirMetal::CONTEXT->managers.textureManager;
        bool viewportResult = texManager->resizeTexture(_device, self.viewportHandle, viewportSize.x, viewportSize.y);
        bool depthResult = texManager->resizeTexture(_device, self.depthHandle, viewportSize.x, viewportSize.y);
        bool jumpResult = texManager->resizeTexture(_device, self.jumpHandle, viewportSize.x, viewportSize.y);
        bool jumpResult2 = texManager->resizeTexture(_device, self.jumpHandle2, viewportSize.x, viewportSize.y);
        bool jumpMaskResult = texManager->resizeTexture(_device, self.jumpMaskHandle, viewportSize.x, viewportSize.y);
        if ((!viewportResult) | (!depthResult) | (!jumpResult)) {
            SIR_CORE_FATAL("Could not resize viewport color or depth buffer");
        } else {
            //if we got no error we extract the new texture so that can be used
            self.offScreenTexture = texManager->getNativeFromHandle(self.viewportHandle);
            self.depthTexture = texManager->getNativeFromHandle(self.depthHandle);
            self.jumpTexture = texManager->getNativeFromHandle(self.jumpHandle);
            self.jumpTexture2 = texManager->getNativeFromHandle(self.jumpHandle2);
            self.jumpMaskTexture = texManager->getNativeFromHandle(self.jumpMaskHandle);
            SirMetal::CONTEXT->viewportTexture = (__bridge void *) self.offScreenTexture;
            updateVoidIndices(viewportSize.x, viewportSize.y, self.floodUniform);
            SirMetal::CONTEXT->cameraController.updateProjection(viewportSize.x, viewportSize.y);

        }
        SirMetal::setFlagBitfield(SirMetal::CONTEXT->flags.viewEvents, SirMetal::ViewEventsFlagsBits::ViewEventsViewportSizeChanged, false);
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

    id <MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
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

    /*
    MTLRenderPassStencilAttachmentDescriptor *stencilAttachment = _metalRenderPassDesc.stencilAttachment;
    stencilAttachment.texture = depthAttachment.texture;
    stencilAttachment.storeAction = MTLStoreActionDontCare;
    stencilAttachment.loadAction = desc.clear ? MTLLoadActionClear : MTLLoadActionLoad;
    */
    m_drawTracker.renderTargets[0] = isViewport ? self.offScreenTexture : view.currentDrawable.texture;
    m_drawTracker.depthTarget = isViewport ? self.depthTexture : self.depthTextureGUI;
    PSOCache cache = getPSO(_device, m_drawTracker, m_opaqueMaterial);

    id <MTLRenderCommandEncoder> renderPass = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];
    [renderPass setRenderPipelineState:cache.color];
    [renderPass setDepthStencilState:cache.depth];
    [renderPass setFrontFacingWinding:MTLWindingCounterClockwise];
    [renderPass setCullMode:MTLCullModeBack];


    const NSUInteger uniformBufferOffset = AlignUp(sizeof(MBEUniforms), MBEBufferAlignment) * self.bufferIndex;

    [renderPass setVertexBuffer:self.uniformBuffer offset:uniformBufferOffset atIndex:1];

    SirMetal::MeshHandle mesh = SirMetal::CONTEXT->managers.meshManager->getHandleFromName("lucy");
    const SirMetal::MeshData *meshData = SirMetal::CONTEXT->managers.meshManager->getMeshData(mesh);

    [renderPass setVertexBuffer:meshData->vertexBuffer offset:0 atIndex:0];
    [renderPass drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                           indexCount:meshData->primitivesCount
                            indexType:MBEIndexType
                          indexBuffer:meshData->indexBuffer
                    indexBufferOffset:0];

    [renderPass endEncoding];

    [self renderSelection:wToUse :hToUse :commandBuffer :&m_drawTracker];

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

        id <MTLRenderCommandEncoder> uiPass = [commandBuffer renderCommandEncoderWithDescriptor:uiPassDescriptor];


        [self renderUI:view :passDescriptor :commandBuffer :uiPass];


        [uiPass endEncoding];

        SirMetal::endFrame(SirMetal::CONTEXT);
    }

    [commandBuffer presentDrawable:view.currentDrawable];

    [commandBuffer addCompletedHandler:^(id <MTLCommandBuffer> commandBuffer) {
        self.bufferIndex = (self.bufferIndex + 1) % MBEInFlightBufferCount;
        dispatch_semaphore_signal(self.displaySemaphore);
    }];

    [commandBuffer commit];


}

- (void)renderSelection:(float)w :(float)h :(id <MTLCommandBuffer>)commandBuffer
        :(SirMetal::DrawTracker *)tracker {

    MTLRenderPassDescriptor *passDescriptor = [[MTLRenderPassDescriptor alloc] init];
    MTLRenderPassColorAttachmentDescriptor *uiColorAttachment = passDescriptor.colorAttachments[0];
    uiColorAttachment.texture = self.jumpMaskTexture;
    uiColorAttachment.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    uiColorAttachment.storeAction = MTLStoreActionStore;
    uiColorAttachment.loadAction = MTLLoadActionClear;
    passDescriptor.renderTargetWidth = w;
    passDescriptor.renderTargetHeight = h;

    id <MTLRenderCommandEncoder> renderPass = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];


    //TODO use a reset function on tracker
    tracker->depthTarget =nil;
    tracker->renderTargets[0]=self.jumpMaskTexture;
    PSOCache cache = getPSO(_device,*tracker,m_jumpFloodMaskMaterial);

    //[renderPass setRenderPipelineState:self.jumpMaskPipelineState];
    [renderPass setRenderPipelineState:cache.color];
    //[renderPass setDepthStencilState:nil];
    [renderPass setFrontFacingWinding:MTLWindingCounterClockwise];
    [renderPass setCullMode:MTLCullModeBack];

    const NSUInteger uniformBufferOffset = AlignUp(sizeof(MBEUniforms), MBEBufferAlignment) * self.bufferIndex;

    [renderPass setVertexBuffer:self.uniformBuffer offset:uniformBufferOffset atIndex:1];

    SirMetal::MeshHandle mesh = SirMetal::CONTEXT->managers.meshManager->getHandleFromName("lucy");
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

    tracker->depthTarget =nil;
    tracker->renderTargets[0]=self.jumpTexture;
    cache = getPSO(_device,*tracker,m_jumpFloodInitMaterial);
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
    id <MTLTexture> outTex = self.jumpTexture;
    while (offset != 1) {
        offset = pow(2, (log2(N) - passIndex - 1));

        //do render here
        MTLRenderPassDescriptor *passDescriptorP = [[MTLRenderPassDescriptor alloc] init];
        colorAttachment = passDescriptorP.colorAttachments[0];
        colorAttachment.texture = passIndex % 2 == 0 ? self.jumpTexture2 : self.jumpTexture;
        colorAttachment.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
        colorAttachment.storeAction = MTLStoreActionStore;
        colorAttachment.loadAction = MTLLoadActionClear;
        passDescriptorP.renderTargetWidth = w;
        passDescriptorP.renderTargetHeight = h;

        id <MTLRenderCommandEncoder> renderPass3 = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptorP];

        [renderPass3 setRenderPipelineState:self.jumpPipelineState];
        [renderPass3 setFrontFacingWinding:MTLWindingClockwise];
        [renderPass3 setCullMode:MTLCullModeNone];
        [renderPass3 setFragmentTexture:passIndex % 2 == 0 ? self.jumpTexture : self.jumpTexture2 atIndex:0];
        //TODO fix hardcoded offset
        [renderPass3 setFragmentBuffer:self.floodUniform offset:9 * 256 + passIndex * 256 atIndex:0];


        //first screen pass
        [renderPass3 drawPrimitives:MTLPrimitiveTypeTriangle
                        vertexStart:0 vertexCount:6];

        [renderPass3 endEncoding];
        outTex = passIndex % 2 == 0 ? self.jumpTexture2 : self.jumpTexture;
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
    passDescriptor.renderTargetWidth = w;
    passDescriptor.renderTargetHeight = h;

    id <MTLRenderCommandEncoder> renderPass4 = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];

    [renderPass4 setRenderPipelineState:self.jumpOutlinePipelineState];
    [renderPass4 setFrontFacingWinding:MTLWindingClockwise];
    [renderPass4 setCullMode:MTLCullModeNone];
    [renderPass4 setFragmentTexture:self.jumpMaskTexture atIndex:0];
    [renderPass4 setFragmentTexture:outTex atIndex:1];
    //we need the screen size from the uniform
    [renderPass4 setFragmentBuffer:self.floodUniform offset:0 atIndex:0];

    [renderPass4 drawPrimitives:MTLPrimitiveTypeTriangle
                    vertexStart:0 vertexCount:6];
    [renderPass4 endEncoding];
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {

    //here we divide by the scale factor so our engine will work with the
    //real sizes if needed and not scaled up views
    CGFloat scaling = view.window.screen.backingScaleFactor;
    SirMetal::CONTEXT->screenWidth = static_cast<uint32_t>(size.width / scaling);
    SirMetal::CONTEXT->screenHeight = static_cast<uint32_t>(size.height / scaling);
}

@end


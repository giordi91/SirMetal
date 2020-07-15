/*
See LICENSE folder for this sample’s licensing information.

Abstract:
Implementation of a platform independent renderer class, which performs Metal setup and per frame rendering
*/

#import "MetalKit/MetalKit.h"

#import "AAPLRenderer.h"
#include "engineContext.h"
#import "MBEMathUtilities.h"

typedef struct
{
    vector_float4 position;
    vector_float4 color;
} MBEVertex;
static const NSInteger MBEInFlightBufferCount = 3;

typedef uint16_t MBEIndex;
const MTLIndexType MBEIndexType = MTLIndexTypeUInt16;

typedef struct
{
    matrix_float4x4 modelViewProjectionMatrix;
} MBEUniforms;

static inline uint64_t AlignUp(uint64_t n, uint32_t alignment) {
    return ((n + alignment - 1) / alignment) * alignment;
}

static const uint32_t MBEBufferAlignment = 256;

@interface AAPLRenderer ()
@property(strong) id <MTLRenderPipelineState> renderPipelineState;
@property(strong) id <MTLDepthStencilState> depthStencilState;
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property (strong) id<MTLBuffer> indexBuffer;
@property (strong) id<MTLBuffer> uniformBuffer;
@property (strong) dispatch_semaphore_t displaySemaphore;
@property (assign) NSInteger bufferIndex;
@property (assign) float rotationX, rotationY, time;
@end

/*
@property (strong) id<MTLDevice> device;
@property (strong) id<MTLBuffer> vertexBuffer;
@property (strong) id<MTLBuffer> indexBuffer;
@property (strong) id<MTLBuffer> uniformBuffer;
@property (strong) id<MTLCommandQueue> commandQueue;
@property (strong) id<MTLRenderPipelineState> renderPipelineState;
@property (strong) id<MTLDepthStencilState> depthStencilState;
@property (strong) dispatch_semaphore_t displaySemaphore;
@property (assign) NSInteger bufferIndex;
@property (assign) float rotationX, rotationY, time;
*/
// Main class performing the rendering
@implementation AAPLRenderer {
    id <MTLDevice> _device;

    // The command queue used to pass commands to the device.
    id <MTLCommandQueue> _commandQueue;
}

- (void) logGPUInformation: (id<MTLDevice>)device
{
    NSLog(@"Device name: %@", device.name);
    NSLog(@"\t\tis low power: %@",device.isLowPower ? @"YES" : @"NO" );
    NSLog(@"\t\tis removable: %@", device.isRemovable ? @"YES" :@"NO");
    NSLog(@"\t\tsupport macOS GPU family 1: %@", [device supportsFeatureSet:MTLFeatureSet::MTLFeatureSet_macOS_GPUFamily1_v1] ? @"YES" :@"NO");
    NSLog(@"\t\tsupport macOS GPU family 2: %@", [device supportsFeatureSet:MTLFeatureSet::MTLFeatureSet_macOS_GPUFamily2_v1] ? @"YES" :@"NO");
}


- (nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)mtkView {
    self = [super init];
    if (self) {
        _device = mtkView.device;
        _displaySemaphore = dispatch_semaphore_create(MBEInFlightBufferCount);


        // Create the command queue
        _commandQueue = [_device newCommandQueue];
    }
    NSLog(@"Initializing Sir Metal: v0.0.1");
    [self logGPUInformation:_device];

    mtkView.paused = NO;


    //create the pipeline
    [self makePipeline];
    [self makeBuffers];


    return self;
}

- (void)makePipeline {

    id <MTLLibrary> library = [_device newDefaultLibrary];

    MTLRenderPipelineDescriptor *pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.vertexFunction = [library newFunctionWithName:@"vertex_project"];
    pipelineDescriptor.fragmentFunction = [library newFunctionWithName:@"fragment_flatcolor"];
    pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

    MTLDepthStencilDescriptor *depthStencilDescriptor = [MTLDepthStencilDescriptor new];
    depthStencilDescriptor.depthCompareFunction = MTLCompareFunctionLess;
    depthStencilDescriptor.depthWriteEnabled = YES;
    self.depthStencilState = [_device newDepthStencilStateWithDescriptor:depthStencilDescriptor];

    NSError *error = nil;
    self.renderPipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDescriptor
                                                                       error:&error];

    if (!self.renderPipelineState) {
        NSLog(@"Error occurred when creating render pipeline state: %@", error);
    }
}
/*
- (void)makePipeline
{
    NSString *shaderPath= [NSString stringWithCString:SirMetal::CONTEXT->projectPath
                                                encoding:[NSString defaultCStringEncoding]];
    __autoreleasing NSError *errorLib = nil;
    shaderPath = [shaderPath  stringByAppendingString:@"/shaders/Shaders.metal"];

    const char *cString = [shaderPath cStringUsingEncoding:NSASCIIStringEncoding];
    const char* shaderData = readFile(cString);

    NSString *content = [NSString stringWithContentsOfFile:shaderPath encoding:NSUTF8StringEncoding error:nil];
    NSLog(shaderPath);
    id<MTLLibrary> libraryRaw = [_device newLibraryWithSource:content options:nil error:&errorLib];

    id<MTLFunction> vertexFunc = [libraryRaw newFunctionWithName:@"vertex_main"];
    id<MTLFunction> fragmentFunc = [libraryRaw newFunctionWithName:@"fragment_main"];

    MTLRenderPipelineDescriptor *pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    pipelineDescriptor.vertexFunction = vertexFunc;
    pipelineDescriptor.fragmentFunction = fragmentFunc;

    NSError *error = nil;
    self.renderPipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDescriptor
                                                       error:&error];

    if (!self.renderPipelineState)
    {
        NSLog(@"Error occurred when creating render pipeline state: %@", error);
    }

    _commandQueue = [_device newCommandQueue];
}
*/


/*
- (void)makeBuffers
{
    static const MBEVertex vertices[] =
            {
                    { .position = {  0.0,  0.5, 0, 1 }, .color = { 1, 0, 0, 1 } },
                    { .position = { -0.5, -0.5, 0, 1 }, .color = { 0, 1, 0, 1 } },
                    { .position = {  0.5, -0.5, 0, 1 }, .color = { 0, 0, 1, 1 } }
            };

    self.vertexBuffer = [_device newBufferWithBytes:vertices
                                        length:sizeof(vertices)
                                       options:MTLResourceOptionCPUCacheModeDefault];
}
*/

- (void)makeBuffers
{
    static const MBEVertex vertices[] =
            {
                    { .position = { -1,  1,  1, 1 }, .color = { 0, 1, 1, 1 } },
                    { .position = { -1, -1,  1, 1 }, .color = { 0, 0, 1, 1 } },
                    { .position = {  1, -1,  1, 1 }, .color = { 1, 0, 1, 1 } },
                    { .position = {  1,  1,  1, 1 }, .color = { 1, 1, 1, 1 } },
                    { .position = { -1,  1, -1, 1 }, .color = { 0, 1, 0, 1 } },
                    { .position = { -1, -1, -1, 1 }, .color = { 0, 0, 0, 1 } },
                    { .position = {  1, -1, -1, 1 }, .color = { 1, 0, 0, 1 } },
                    { .position = {  1,  1, -1, 1 }, .color = { 1, 1, 0, 1 } }
            };

    static const MBEIndex indices[] =
            {
                    3, 2, 6, 6, 7, 3,
                    4, 5, 1, 1, 0, 4,
                    4, 0, 3, 3, 7, 4,
                    1, 5, 6, 6, 2, 1,
                    0, 1, 2, 2, 3, 0,
                    7, 6, 5, 5, 4, 7
            };

    _vertexBuffer = [_device newBufferWithBytes:vertices
                                             length:sizeof(vertices)
                                            options:MTLResourceOptionCPUCacheModeDefault];
    [_vertexBuffer setLabel:@"Vertices"];

    _indexBuffer = [_device newBufferWithBytes:indices
                                            length:sizeof(indices)
                                           options:MTLResourceOptionCPUCacheModeDefault];
    [_indexBuffer setLabel:@"Indices"];

    _uniformBuffer = [_device newBufferWithLength:AlignUp(sizeof(MBEUniforms), MBEBufferAlignment) * MBEInFlightBufferCount
                                              options:MTLResourceOptionCPUCacheModeDefault];
    [_uniformBuffer setLabel:@"Uniforms"];
}

- (void)updateUniformsForView: (float) screenWidth: (float) screenHeight
{
    float duration = 0.01;
    self.time += duration;
    self.rotationX += duration * (M_PI / 2);
    self.rotationY += duration * (M_PI / 3);
    float scaleFactor = sinf(5 * self.time) * 0.25 + 1;
    const vector_float3 xAxis = { 1, 0, 0 };
    const vector_float3 yAxis = { 0, 1, 0 };
    const matrix_float4x4 xRot = matrix_float4x4_rotation(xAxis, self.rotationX);
    const matrix_float4x4 yRot = matrix_float4x4_rotation(yAxis, self.rotationY);
    const matrix_float4x4 scale = matrix_float4x4_uniform_scale(scaleFactor);
    const matrix_float4x4 modelMatrix = matrix_multiply(matrix_multiply(xRot, yRot), scale);

    const vector_float3 cameraTranslation = { 0, 0, -5 };
    const matrix_float4x4 viewMatrix = matrix_float4x4_translation(cameraTranslation);

    const float aspect = screenWidth / screenHeight;
    const float fov = (2 * M_PI) / 5;
    const float near = 1;
    const float far = 100;
    const matrix_float4x4 projectionMatrix = matrix_float4x4_perspective(aspect, fov, near, far);

    MBEUniforms uniforms;
    uniforms.modelViewProjectionMatrix = matrix_multiply(projectionMatrix, matrix_multiply(viewMatrix, modelMatrix));

    const NSUInteger uniformBufferOffset = AlignUp(sizeof(MBEUniforms), MBEBufferAlignment) * self.bufferIndex;
    memcpy((char*)([self.uniformBuffer contents]) + uniformBufferOffset, &uniforms, sizeof(uniforms));
}

/// Called whenever the view needs to render a frame.
- (void)drawInMTKView:(nonnull MTKView *)view {

    /*
    // The render pass descriptor references the texture into which Metal should draw
    MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;
    if (renderPassDescriptor == nil) {
        return;
    }

    //id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];

    // Create a render pass and immediately end encoding, causing the drawable to be cleared
    //id<MTLRenderCommandEncoder> commandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];

    //[commandEncoder endEncoding];

    // Get the drawable that will be presented at the end of the frame
    id <CAMetalDrawable> drawable = view.currentDrawable;
    id <MTLTexture> texture = drawable.texture;

    if (drawable)
    {
        MTLRenderPassDescriptor *passDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
        passDescriptor.colorAttachments[0].texture = texture;
        passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.85, 0.85, 0.85, 1);
        passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;

        id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];

        id<MTLRenderCommandEncoder> commandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];
        [commandEncoder setRenderPipelineState:self.renderPipelineState];
        [commandEncoder setVertexBuffer:self.vertexBuffer offset:0 atIndex:0];
        [commandEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        [commandEncoder endEncoding];

        [commandBuffer presentDrawable:drawable];
        [commandBuffer commit];
    }
    */
    //dispatch_semaphore_wait(self.displaySemaphore, DISPATCH_TIME_FOREVER);

    view.clearColor = MTLClearColorMake(0.95, 0.95, 0.95, 1);

    float w = view.drawableSize.width;
    float h = view.drawableSize.height;
    [self updateUniformsForView:w :h];
    NSLog(@"render");

    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];

    MTLRenderPassDescriptor *passDescriptor = [view currentRenderPassDescriptor];

    id<MTLRenderCommandEncoder> renderPass = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];
    [renderPass setRenderPipelineState:self.renderPipelineState];
    [renderPass setDepthStencilState:self.depthStencilState];
    [renderPass setFrontFacingWinding:MTLWindingCounterClockwise];
    [renderPass setCullMode:MTLCullModeBack];

    const NSUInteger uniformBufferOffset = AlignUp(sizeof(MBEUniforms), MBEBufferAlignment) * self.bufferIndex;

    [renderPass setVertexBuffer:self.vertexBuffer offset:0 atIndex:0];
    [renderPass setVertexBuffer:self.uniformBuffer offset:uniformBufferOffset atIndex:1];

    [renderPass drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                           indexCount:[self.indexBuffer length] / sizeof(MBEIndex)
                            indexType:MBEIndexType
                          indexBuffer:self.indexBuffer
                    indexBufferOffset:0];

    [renderPass endEncoding];

    [commandBuffer presentDrawable:view.currentDrawable ];

    //[commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer) {
    //    self.bufferIndex = (self.bufferIndex + 1) % MBEInFlightBufferCount;
    //    dispatch_semaphore_signal(self.displaySemaphore);
    //}];

    [commandBuffer commit];


}


/// Called whenever view changes orientation or is resized
- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {
}

@end


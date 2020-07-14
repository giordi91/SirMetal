/*
See LICENSE folder for this sample’s licensing information.

Abstract:
Implementation of a platform independent renderer class, which performs Metal setup and per frame rendering
*/

#import "MetalKit/MetalKit.h"

#import "AAPLRenderer.h"
#include "engineContext.h"

typedef struct
{
    vector_float4 position;
    vector_float4 color;
} MBEVertex;

@interface AAPLRenderer ()
@property(strong) id <MTLRenderPipelineState> renderPipelineState;
@property(strong) id <MTLDepthStencilState> depthStencilState;
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property NSString* workingDirectory;
@end

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

        // Create the command queue
        _commandQueue = [_device newCommandQueue];
    }
    NSLog(@"Initializing Sir Metal: v0.0.1");
    //initialize working directory
    NSFileManager *filemgr;

    filemgr = [[NSFileManager alloc] init];

    self.workingDirectory = [filemgr currentDirectoryPath];
    NSLog(@"Working directory %@", self.workingDirectory);

    [self logGPUInformation:_device];



    //create the pipeline
    [self makePipeline];
    [self makeBuffers];


    return self;
}

/*
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
 */
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

/// Called whenever the view needs to render a frame.
- (void)drawInMTKView:(nonnull MTKView *)view {
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

    /*
    MTLRenderPassDescriptor *passDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    passDescriptor.colorAttachments[0].texture = texture;
    passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(1, 0, 0, 1);

    id <MTLCommandQueue> commandQueue = [_device newCommandQueue];

    id <MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];

    id <MTLRenderCommandEncoder> commandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];
    [commandEncoder endEncoding];

    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
     */
}


/// Called whenever view changes orientation or is resized
- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {
}

@end


/*
See LICENSE folder for this sample’s licensing information.

Abstract:
Header for a platform independent renderer class, which performs Metal setup and per frame rendering.
*/

#import <AppKit/AppKit.h>
#import "MetalKit/MetalKit.h"
#import "SirMetalLib/resources/handle.h"


@interface AAPLRenderer : NSObject<MTKViewDelegate>

@property(nonatomic) SirMetal::TextureHandle viewportHandle;
@property(nonatomic) SirMetal::TextureHandle depthHandle;

@property(nonatomic) CGFloat screenHeight;

@property(nonatomic) CGFloat screenWidth;


- (nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)mtkView;
- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size;
- (void)initGraphicsObjectsTemp;

@end


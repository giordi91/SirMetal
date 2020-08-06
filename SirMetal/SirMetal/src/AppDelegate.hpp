
#ifndef METALWINDOW_APPDELEGATE_HPP
#define METALWINDOW_APPDELEGATE_HPP


#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>
#import "AAPLRenderer.h"

@interface SirMTKView: MTKView
-(void)customInit;
@end

@interface AppDelegate : NSObject <NSApplicationDelegate>

@property (strong) NSWindow *window;
@property (strong) AAPLRenderer *renderer;
@property (readonly) id<MTLDevice> device;

@end

#endif //METALWINDOW_APPDELEGATE_HPP

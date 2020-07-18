
#ifndef METALWINDOW_APPDELEGATE_HPP
#define METALWINDOW_APPDELEGATE_HPP


#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>

@interface SirMTKView: MTKView
@end

@interface AppDelegate : NSObject <NSApplicationDelegate>

@property (assign) NSWindow *window;
@property (readonly) id<MTLDevice> device;

@end

#endif //METALWINDOW_APPDELEGATE_HPP

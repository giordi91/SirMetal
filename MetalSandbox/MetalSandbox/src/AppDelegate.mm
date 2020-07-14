//
// Created by Dhruv Govil on 2019-09-01.
//

#include <iostream>
#import "AppDelegate.hpp"
#import "AAPLRenderer.h"

@implementation AppDelegate

@synthesize window;

- (void)dealloc
{
    [super dealloc];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
//    Create Main Window
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    window  = [[[NSWindow alloc] initWithContentRect:frame
                                                     styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                                       backing:NSBackingStoreBuffered
                                                         defer:NO] autorelease];
    [window setBackgroundColor:[NSColor darkGrayColor]];
    [window makeKeyAndOrderFront:NSApp];
    //seems to avoid crash on exit, go figure :D gotta love this language
    [window setReleasedWhenClosed: NO];

    std::cout << "Finished Launching Application" << std::endl;

//    Create Metal View

    //NSView *view = [[NSView alloc] initWithFrame:NSMakeRect(100, 100, 100, 100)];
    MTKView *view = [[MTKView alloc] initWithFrame:NSMakeRect(000, 0, 800, 600)];
    [view setWantsLayer:YES];
    view.enableSetNeedsDisplay = YES;
    view.device = MTLCreateSystemDefaultDevice();
    view.clearColor = MTLClearColorMake(0.0, 0.5, 1.0, 1.0);

    [self.window.contentView addSubview:view];

    AAPLRenderer* _renderer = [[AAPLRenderer alloc] initWithMetalKitView:view];
    if(!_renderer)
    {
        NSLog(@"Renderer initialization failed");
        return;
    }

    // Initialize the renderer with the view size.
    [_renderer mtkView:view drawableSizeWillChange:view.drawableSize];
    view.delegate = _renderer;

    NSLog(@"subviews are: %@", [self.window.contentView subviews]);

}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication {
    return YES;
}

@end

//
// Created by Dhruv Govil on 2019-09-01.
//

#include <iostream>
#import "vendors/imgui/imgui_impl_osx.h"
#import "AppDelegate.hpp"
#import "AAPLRenderer.h"
#import "imgui.h"

@implementation SirMTKView
-(void) customInit {
    NSLog(@"YOOOOOOO");
    auto context = ImGui::CreateContext();
    ImGui::SetCurrentContext(context);
// Add a tracking area in order to receive mouse events whenever the mouse is within the bounds of our view
    NSTrackingArea *trackingArea = [[NSTrackingArea alloc] initWithRect:NSZeroRect
                                                                options:NSTrackingMouseMoved | NSTrackingInVisibleRect | NSTrackingActiveAlways
                                                                  owner:self
                                                               userInfo:nil];
    [self addTrackingArea:trackingArea];

// If we want to receive key events, we either need to be in the responder chain of the key view,
// or else we can install a local monitor. The consequence of this heavy-handed approach is that
// we receive events for all controls, not just Dear ImGui widgets. If we had native controls in our
// window, we'd want to be much more careful than just ingesting the complete event stream, though we
// do make an effort to be good citizens by passing along events when Dear ImGui doesn't want to capture.
    NSEventMask eventMask = NSEventMaskKeyDown | NSEventMaskKeyUp | NSEventMaskFlagsChanged | NSEventTypeScrollWheel;
    [NSEvent addLocalMonitorForEventsMatchingMask:eventMask handler:^NSEvent *_Nullable(NSEvent *event) {
        BOOL wantsCapture = ImGui_ImplOSX_HandleEvent(event, self);
        if (event.type == NSEventTypeKeyDown && wantsCapture) {
            return nil;
        } else {
            return event;
        }

    }];

    ImGui_ImplOSX_Init();
}

-(void)mouseMoved:(NSEvent *)event {
    ImGui_ImplOSX_HandleEvent(event, self);
}

- (void)mouseDown:(NSEvent *)event {
    ImGui_ImplOSX_HandleEvent(event, self);
    ImGuiIO &io = ImGui::GetIO();
    io.MouseDown[static_cast<int>(event.buttonNumber)] = true;
}

- (void)rightMouseDown:(NSEvent *)event {
    ImGui_ImplOSX_HandleEvent(event, self);
}

- (void)otherMouseDown:(NSEvent *)event {
    ImGui_ImplOSX_HandleEvent(event, self);
}

- (void)mouseUp:(NSEvent *)event {
    ImGui_ImplOSX_HandleEvent(event, self);
}

- (void)rightMouseUp:(NSEvent *)event {
    ImGui_ImplOSX_HandleEvent(event, self);
}

- (void)otherMouseUp:(NSEvent *)event {
    ImGui_ImplOSX_HandleEvent(event, self);
}

- (void)mouseDragged:(NSEvent *)event {
    ImGui_ImplOSX_HandleEvent(event, self);
}

- (void)rightMouseDragged:(NSEvent *)event {
    ImGui_ImplOSX_HandleEvent(event, self);
}

- (void)otherMouseDragged:(NSEvent *)event {
    ImGui_ImplOSX_HandleEvent(event, self);
}

- (void)scrollWheel:(NSEvent *)event {
    ImGui_ImplOSX_HandleEvent(event, self);
}
- (BOOL)acceptsFirstResponder
{
    return YES;
}
@end

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
    MTKView *view = [[SirMTKView alloc] initWithFrame:NSMakeRect(000, 0, 800, 600)];
    [view setWantsLayer:YES];
    view.enableSetNeedsDisplay = YES;
    view.device = MTLCreateSystemDefaultDevice();
    view.clearColor = MTLClearColorMake(0.0, 0.5, 1.0, 1.0);
    [view customInit];

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

    [ window  makeFirstResponder:view];
    NSLog(@"subviews are: %@", [self.window.contentView subviews]);

}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication {
    return YES;
}



@end

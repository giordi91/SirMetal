//
// Created by Dhruv Govil on 2019-09-01.
//

#include <iostream>
#import <AppKit/AppKit.h>
#import "vendors/imgui/imgui_impl_osx.h"
#import "AppDelegate.hpp"
#import "AAPLRenderer.h"
#import "core/input.h"
#import "engineContext.h"
#import "log.h"

@implementation SirMTKView
- (void)customInit {
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

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui_ImplOSX_Init();
}

- (void)mouseMoved:(NSEvent *)event {
    ImGui_ImplOSX_HandleEvent(event, self);
    NSPoint mousePoint = event.locationInWindow;
    SirMetal::CONTEXT->input.setMousePos(mousePoint.x, mousePoint.y);
    SirMetal::CONTEXT->input.setMouseDelta(static_cast<float>(event.deltaX), static_cast<float>(event.deltaY));
}

- (void)mouseDown:(NSEvent *)event {
    ImGui_ImplOSX_HandleEvent(event, self);
    ImGuiIO &io = ImGui::GetIO();
    io.MouseDown[static_cast<int>(event.buttonNumber)] = true;
    SirMetal::CONTEXT->input.setMouse(SirMetal::MOUSE_BUTTONS::LEFT, 1);
}

- (void)rightMouseDown:(NSEvent *)event {
    ImGui_ImplOSX_HandleEvent(event, self);
    SirMetal::CONTEXT->input.setMouse(SirMetal::MOUSE_BUTTONS::RIGHT, 1);
}

- (void)otherMouseDown:(NSEvent *)event {
    ImGui_ImplOSX_HandleEvent(event, self);
}

- (void)mouseUp:(NSEvent *)event {
    ImGui_ImplOSX_HandleEvent(event, self);
    SirMetal::CONTEXT->input.setMouse(SirMetal::MOUSE_BUTTONS::LEFT, 0);

}

- (void)rightMouseUp:(NSEvent *)event {
    ImGui_ImplOSX_HandleEvent(event, self);
    SirMetal::CONTEXT->input.setMouse(SirMetal::MOUSE_BUTTONS::RIGHT, 0);
}

- (void)otherMouseUp:(NSEvent *)event {
    ImGui_ImplOSX_HandleEvent(event, self);
}

- (void)mouseDragged:(NSEvent *)event {
    ImGui_ImplOSX_HandleEvent(event, self);
    NSPoint mousePoint = event.locationInWindow;
    SirMetal::CONTEXT->input.setMousePos(mousePoint.x, mousePoint.y);
    SirMetal::CONTEXT->input.setMouseDelta(static_cast<float>(event.deltaX), static_cast<float>(event.deltaY));
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

- (void)keyDown:(NSEvent *)event {
    NSLog(@"key down %i", event.keyCode);
    //53 == esc
    if (event.keyCode == 53) {
        [self.window close];
        return;
    }
    if (event.keyCode == 50)//~
    {
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags ^= ImGuiConfigFlags_DockingEnable;
    }
    SirMetal::CONTEXT->input.keyDown(event.keyCode);
    //to detect shift do
    //if([event modifierFlags] & NSEventModifierFlagShift)
    //{
    //}

}

- (void)keyUp:(NSEvent *)event {
    SirMetal::CONTEXT->input.keyUp(event.keyCode);
}

- (BOOL)acceptsFirstResponder {
    return YES;
}
@end

@implementation AppDelegate

@synthesize window;

- (void)dealloc {
    [super dealloc];
}


- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
//    Create Main Window


    NSScreen *screen = NSScreen.mainScreen;
    NSRect screenRect = [screen visibleFrame];
    NSLog(@"Screen size %.1f-%.1f %.1fx%.1f", screenRect.origin.x, screenRect.origin.y, screenRect.size.width, screenRect.size.height);

    //2880.0x1480.0
    //2880.0x1594.0
    //screenRect = NSMakeRect(0, 0, 1440, 795);

    window = [[[NSWindow alloc] initWithContentRect:screenRect
                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                            backing:NSBackingStoreBuffered
                                              defer:NO] autorelease];
    //screenRect = [window constrainFrameRect:];
    [window setBackgroundColor:[NSColor darkGrayColor]];
    [window makeKeyAndOrderFront:NSApp];
    //seems to avoid crash on exit, go figure :D gotta love this language
    [window setReleasedWhenClosed:NO];

    std::cout << "Finished Launching Application" << std::endl;

//    Create Metal View

    NSRect viewSize = screenRect;
    viewSize.origin.y = 0;
    viewSize.size.height -= 20;

    NSLog(@"Screen size %.1f-%.1f %.1fx%.1f", viewSize.origin.x, viewSize.origin.y, viewSize.size.width, viewSize.size.height);
    MTKView *view = [[SirMTKView alloc] initWithFrame:viewSize];
    [view setWantsLayer:YES];
    view.enableSetNeedsDisplay = YES;
    view.device = MTLCreateSystemDefaultDevice();
    view.clearColor = MTLClearColorMake(0.0, 0.5, 1.0, 1.0);
    [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

    [self.window.contentView addSubview:view];
    [view customInit];

    AAPLRenderer *_renderer = [[AAPLRenderer alloc] initWithMetalKitView:view];
    if (!_renderer) {
        NSLog(@"Renderer initialization failed");
        return;
    }

    // Initialize the renderer with the view size.
    [_renderer mtkView:view drawableSizeWillChange:view.drawableSize];
    view.delegate = _renderer;
    view.autoResizeDrawable = true;
    view.autoresizesSubviews = true;

    [window makeFirstResponder:view];
    NSLog(@"subviews are: %@", [self.window.contentView subviews]);

}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication {
    return YES;
}


@end

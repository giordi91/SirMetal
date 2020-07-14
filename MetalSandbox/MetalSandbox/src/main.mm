#include <iostream>
#import <Cocoa/Cocoa.h>
#import "AppDelegate.hpp"
#include "engineContext.h"

int main(int argc, const char * argv[]) {
    
    std::cout << "Starting application" << std::endl;

    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    NSApplication *NSApp = [NSApplication sharedApplication];

    SirMetal::initializeContext(argv[1]);

    AppDelegate *appDelegate = [[AppDelegate alloc] init];
    [NSApp setDelegate:appDelegate];
    [NSApp run];
    [pool release];
    return 0;
}

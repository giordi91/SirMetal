
#include "project.h"
#import "log.h"
#include <string>

#import <AppKit/AppKit.h>

namespace SirMetal {
    namespace Editor {
        //Open a dialog to get a project path file from the user
        NSString* getProjectPathFromUser() {
            NSOpenPanel *panel;
            NSArray *fileTypes = [NSArray arrayWithObjects:@"SirMetalProject", nil];
            panel = [NSOpenPanel openPanel];
            [panel setFloatingPanel:YES];
            [panel setCanChooseDirectories:NO];
            [panel setCanChooseFiles:YES];
            [panel setAllowsMultipleSelection:YES];
            [panel setAllowedFileTypes:fileTypes];
            int i = [panel runModal];
            if (i == NSModalResponseOK) {
                for (NSURL *URL in [panel URLs]) {
                    NSLog(@"%@", [URL path]);
                    NSString *urlString = URL.path;
                    return urlString;
                }
            }
            return nil;
        }

        std::string getProjectPathCached() {

            //We use the NSUserDefaults to store a last open project
            //if it sets we open that one, the user will be able to open another
            //project later if needed
            //otherwise we pop up a dialog to ask to get the path to a project
            NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];// getting an NSString
            NSString *cachedPath = [prefs stringForKey:@"lastProjectPath"];
            std::string toReturn;
            if(cachedPath == nil)
            {
                NSString*  path = getProjectPathFromUser();
                [prefs setObject: path forKey: @"lastProjectPath"];
                toReturn =  std::string([path UTF8String]);
            } else{
                toReturn =  std::string([cachedPath UTF8String]);
            }
            return toReturn;
        }

        void initializeProject() {
            const std::string path = getProjectPathCached();
            SIR_CORE_INFO("Opening project at path {}", path);
        }
    }
}

#include "project.h"
#import "log.h"
#include <string>

#import <AppKit/AppKit.h>

#include <filesystem>

#include "../platform/file.h"
#include "../core/jsonParse.h"

namespace SirMetal {
    namespace Editor {
        namespace PROJECT_KEY {
            static const std::string defaultProjectName = "Empty Project";
            static const std::string projectName = "projectName";
        }

        namespace CAMERA_KEY {
            static const CameraManipulationConfig defaultValues = {1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 0.2f, 0.005f};
            static const std::string lrLookDir = "leftRightLookDirection";
            static const std::string udLookDir = "upDownLookDirection";
            static const std::string lrMovDir = "leftRightMovementDirection";
            static const std::string fbMovDir = "forwardBackMovementDirection";
            static const std::string udMovDir = "upDownMovementDirection";
            static const std::string movSpeed = "movementSpeed";
            static const std::string lookSpeed = "lookSpeed";
        }


        Project *PROJECT = nullptr;


        //Open a dialog to get a project path file from the user
        NSString *getProjectPathFromUser() {
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
            if (cachedPath == nil) {
                NSString *path = getProjectPathFromUser();
                [prefs setObject:path forKey:@"lastProjectPath"];
                toReturn = std::string([path UTF8String]);
            } else {
                toReturn = std::string([cachedPath UTF8String]);
            }
            return toReturn;
        }

        bool initializeProject() {
            const std::string path = getProjectPathCached();
            std::string editorLogPath = getPathName(path) += "/editor.log";
            Log::init(editorLogPath);

            PROJECT = new Project();
            SIR_CORE_INFO("Opening project at path {}", path);
            return PROJECT->initialize(path);
        }

        bool Project::initialize(const std::string &path) {
            m_projectFilePath = path;
            m_projectPath = getPathName(path);

            //let us read up the content of the project
            return parseProjectFile(path);
        }

        bool Project::parseProjectFile(const std::string &path) {

            const nlohmann::json jobj = getJsonObj(path);
            if (jobj.empty()) {
                return false;
            }
            m_settings.m_projectName = getValueIfInJson(jobj, PROJECT_KEY::projectName, PROJECT_KEY::defaultProjectName);
            bool result = parseCameraSettings(jobj);
            if (!result) {
                SIR_CORE_ERROR("Could not parse camera settings, default will be used");
            }
            return true;
        }

        bool Project::parseCameraSettings(const nlohmann::json &jobj) {
            m_settings.m_cameraConfig.leftRightLookDirection = getValueIfInJson(jobj,
                    CAMERA_KEY::lrLookDir, CAMERA_KEY::defaultValues.leftRightLookDirection);
            m_settings.m_cameraConfig.upDownLookDirection = getValueIfInJson(jobj,
                    CAMERA_KEY::udLookDir, CAMERA_KEY::defaultValues.upDownLookDirection);
            m_settings.m_cameraConfig.leftRightMovementDirection = getValueIfInJson(jobj,
                    CAMERA_KEY::lrMovDir, CAMERA_KEY::defaultValues.leftRightMovementDirection);
            m_settings.m_cameraConfig.forwardBackMovementDirection = getValueIfInJson(jobj,
                    CAMERA_KEY::fbMovDir, CAMERA_KEY::defaultValues.forwardBackMovementDirection);
            m_settings.m_cameraConfig.upDownMovementDirection = getValueIfInJson(jobj,
                    CAMERA_KEY::udMovDir, CAMERA_KEY::defaultValues.upDownMovementDirection);
            m_settings.m_cameraConfig.movementSpeed = getValueIfInJson(jobj,
                    CAMERA_KEY::movSpeed, CAMERA_KEY::defaultValues.movementSpeed);
            m_settings.m_cameraConfig.lookSpeed = getValueIfInJson(jobj, CAMERA_KEY::lookSpeed,
                    CAMERA_KEY::defaultValues.lookSpeed);
            return true;
        }
    }
}
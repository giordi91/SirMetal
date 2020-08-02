
#include "project.h"
#import "log.h"
#include <string>

#import <AppKit/AppKit.h>

#include <filesystem>

#include "file.h"
#include "../core/jsonParse.h"
#include "../resources/shaderManager.h"
#include "../resources/meshes/meshManager.h"
#import "engineContext.h"


namespace SirMetal {
    namespace Editor {
        const std::string Project::CACHE_FOLDER_NAME = "cache";
        const std::unordered_set<std::string> IGNORE_FILES = {".DS_Store", "editor.log"};
        const std::unordered_set<std::string> IGNORE_EXT = {".SirMetalProject", ".log"};


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
            static const std::string cameraSettings = "cameraSettings";
        }


        Project *PROJECT = nullptr;


        //Open a dialog to get a project path file from the user
        NSString *getProjectPathFromUser() {
            NSOpenPanel *panel;
            NSArray *fileTypes = @[@"SirMetalProject"];
            panel = [NSOpenPanel openPanel];
            [panel setFloatingPanel:YES];
            [panel setCanChooseDirectories:NO];
            [panel setCanChooseFiles:YES];
            [panel setAllowsMultipleSelection:YES];
            [panel setAllowedFileTypes:fileTypes];
            NSModalResponse response = [panel runModal];
            if (response == NSModalResponseOK) {
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
            m_projectCachePath = m_projectPath + "/" + Project::CACHE_FOLDER_NAME;
            ensureDirectory(m_projectPath);

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

            if (!inJson(jobj, CAMERA_KEY::cameraSettings)) {
                m_settings.m_cameraConfig = CAMERA_KEY::defaultValues;
                SIR_CORE_WARN("Could not find camera settings in project");
                return true;
            }

            const auto &jsetting = jobj[CAMERA_KEY::cameraSettings];


            m_settings.m_cameraConfig.leftRightLookDirection = getValueIfInJson(jsetting,
                    CAMERA_KEY::lrLookDir, CAMERA_KEY::defaultValues.leftRightLookDirection);
            m_settings.m_cameraConfig.upDownLookDirection = getValueIfInJson(jsetting,
                    CAMERA_KEY::udLookDir, CAMERA_KEY::defaultValues.upDownLookDirection);
            m_settings.m_cameraConfig.leftRightMovementDirection = getValueIfInJson(jsetting,
                    CAMERA_KEY::lrMovDir, CAMERA_KEY::defaultValues.leftRightMovementDirection);
            m_settings.m_cameraConfig.forwardBackMovementDirection = getValueIfInJson(jsetting,
                    CAMERA_KEY::fbMovDir, CAMERA_KEY::defaultValues.forwardBackMovementDirection);
            m_settings.m_cameraConfig.upDownMovementDirection = getValueIfInJson(jsetting,
                    CAMERA_KEY::udMovDir, CAMERA_KEY::defaultValues.upDownMovementDirection);
            m_settings.m_cameraConfig.movementSpeed = getValueIfInJson(jsetting,
                    CAMERA_KEY::movSpeed, CAMERA_KEY::defaultValues.movementSpeed);
            m_settings.m_cameraConfig.lookSpeed = getValueIfInJson(jsetting, CAMERA_KEY::lookSpeed,
                    CAMERA_KEY::defaultValues.lookSpeed);
            return true;
        }

        void Project::save() {
            nlohmann::json jobj;
            //saving camera settings
            jobj[PROJECT_KEY::projectName] = m_settings.m_projectName;
            saveCameraSettings(jobj);

            const std::string data = jobj.dump(4);

            const std::string &tempSave = m_projectFilePath + "temp";
            std::ofstream out(tempSave);
            out << data;
            out.close();
            //removing old file
            std::__fs::filesystem::remove(m_projectFilePath);
            //removing moving new file to correct position
            std::__fs::filesystem::rename(tempSave, m_projectFilePath);

        }

        void Project::saveCameraSettings(nlohmann::json &jobj) {
            CameraManipulationConfig &settings = m_settings.m_cameraConfig;

            auto &jsettings = jobj[CAMERA_KEY::cameraSettings];
            jsettings[CAMERA_KEY::lrLookDir] = settings.leftRightLookDirection;
            jsettings[CAMERA_KEY::udLookDir] = settings.upDownLookDirection;
            jsettings[CAMERA_KEY::lrMovDir] = settings.leftRightMovementDirection;
            jsettings[CAMERA_KEY::fbMovDir] = settings.forwardBackMovementDirection;
            jsettings[CAMERA_KEY::udMovDir] = settings.upDownMovementDirection;
            jsettings[CAMERA_KEY::movSpeed] = settings.movementSpeed;
            jsettings[CAMERA_KEY::lookSpeed] = settings.lookSpeed;
        }

        bool isFileOfInterestToProject(const std::string &sourcePath) {


            bool isDir = isPathDirectory(sourcePath);
            if (isDir) {return false;}
            std::string fileName = getFileName(sourcePath);
            bool isIgnoreFile = IGNORE_FILES.find(fileName) != IGNORE_FILES.end();
            if (isIgnoreFile) {
                return false;
            }

            std::string ext = getFileExtension(sourcePath);
            bool isIgnoreExt = IGNORE_EXT.find(ext) != IGNORE_EXT.end();
            if (isIgnoreExt) {return false;}
            return true;
        }


        bool Project::processProjectAssets() {

            using recursive_directory_iterator = std::__fs::filesystem::recursive_directory_iterator;
            for (const auto &dirEntry : recursive_directory_iterator(m_projectPath)) {

                const std::string &path = dirEntry.path().string();
                bool result = isFileOfInterestToProject(path);
                if (!result) {continue;}

                SIR_CORE_INFO("file to load {}", path);
                const std::string &extString = getFileExtension(path);
                FILE_EXT ext = getFileExtFromStr(extString);

                switch (ext) {
                    case FILE_EXT::OBJ: {
                        CONTEXT->managers.meshManager->loadMesh(path);
                        break;
                    }
                    case FILE_EXT::METAL: {
                        CONTEXT->managers.shaderManager->loadShader(path.c_str());
                        break;
                    }
                    default: {
                        SIR_CORE_WARN("Unsupported file ext: {}", extString);
                    }
                }
            }
            return true;
        }

    }
}
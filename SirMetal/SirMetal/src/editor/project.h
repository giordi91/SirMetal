#pragma once

#import <experimental/string>
#import "settings.h"
#import "json.hpp"

namespace SirMetal {
    namespace Editor {
        class Project {

        public:
            bool initialize(const std::string &path);
            const std::string& getProjectFilePath() const{
                return m_projectFilePath;
            }
            const std::string& getProjectPath() const{
                return m_projectPath;
            }
            const Settings& getSettings() const{
                return m_settings;
            }
        private:
            bool parseProjectFile(const std::string& path);
            bool parseCameraSettings(const nlohmann::json& jobj);
        private:
            std::string m_projectFilePath;
            std::string m_projectPath;
            Settings m_settings{};

        };

        bool initializeProject();
        extern Project *PROJECT;

    }
}

#pragma once

#import <experimental/string>
#import <unordered_set>
#import "settings.h"
#import "json/json.hpp"

namespace SirMetal {
    namespace Editor {

        class Project {

        public:
            bool initialize(const std::string &path);

            const std::string &getProjectFilePath() const {
                return m_projectFilePath;
            }

            const std::string &getProjectPath() const {
                return m_projectPath;
            }

            Settings &getSettings() {
                return m_settings;
            }

            void save();

            bool processProjectAssets();

        public:
            static const std::string CACHE_FOLDER_NAME;

        private:
            bool parseProjectFile(const std::string &path);

            bool parseCameraSettings(const nlohmann::json &jobj);

            void saveCameraSettings(nlohmann::json &json);

        private:
            std::string m_projectFilePath;
            std::string m_projectPath;
            std::string m_projectCachePath;
            Settings m_settings{};


        };

        bool initializeProject();

        extern Project *PROJECT;

    }
}

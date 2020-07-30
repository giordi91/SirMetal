#pragma once

#import <experimental/string>

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
        private:
            std::string m_projectFilePath;
            std::string m_projectPath;
        };

        bool initializeProject();
        extern Project *PROJECT;

    }
}

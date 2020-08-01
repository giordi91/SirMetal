#pragma once

#include <filesystem>

namespace SirMetal {

    inline void listFilesInFolder(const char *folderPath,
            std::vector<std::string> &filePaths,
            std::string extension = "NONE") {
        bool shouldFilter = extension != "NONE";
        std::string _extension = "." + extension;
        auto program_p = std::__fs::filesystem::path(folderPath);
        auto dirIt = std::__fs::filesystem::directory_iterator(program_p);
        for (auto p : dirIt) {
            bool isDir = std::__fs::filesystem::is_directory(p);
            if (!isDir) {
                auto path = std::__fs::filesystem::path(p);

                if (shouldFilter && !(path.extension() == _extension)) {
                    continue;
                }
                filePaths.push_back(path.u8string());
            }
        }
    }

    inline std::string getFileName(const std::string &path) {
        const auto expPath = std::__fs::filesystem::path(path);
        return expPath.stem().string();
    }

    inline std::string getFileExtension(const std::string &path) {
        const auto expPath = std::__fs::filesystem::path(path);
        return expPath.extension().string();
    }

    inline std::string getPathName(const std::string &path) {
        const auto expPath = std::__fs::filesystem::path(path);
        return expPath.parent_path().string();
    }

    inline bool fileExists(const std::string &name) {
        return std::__fs::filesystem::exists(name);
    }

    inline bool filePathExists(const std::string &name) {
        const std::__fs::filesystem::path path(name);
        const std::__fs::filesystem::path parent = path.parent_path();
        return std::__fs::filesystem::exists(parent);
    }

    inline bool isPathDirectory(const std::string &name) {
        return std::__fs::filesystem::is_directory(name);
    }

    inline void ensureDirectory(const std::string &path) {
        if (!std::__fs::filesystem::is_directory(path) || !std::__fs::filesystem::exists(path)) { // Check if src folder exists
            std::__fs::filesystem::create_directory(path); // create src folder
        }
    }
}
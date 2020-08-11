#pragma once

#include <filesystem>

namespace SirMetal {

    enum class FILE_EXT {
        NONE = 0,
        OBJ = 1,
        METAL = 2
    };

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
    FILE_EXT getFileExtFromStr(const std::string &ext);
}
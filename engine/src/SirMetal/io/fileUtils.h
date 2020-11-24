#pragma once
#include <string>
#include <vector>

namespace SirMetal {

// basic utilities for manipulating files and  folders, wrapper around CPP
// filesystem
void listFilesInFolder(const char *folderPath,
                       std::vector<std::string> &filePaths,
                       const std::string &wantedExtension = "");

std::string getFileName(const std::string &path);

std::string getFileExtension(const std::string &path);

std::string getPathName(const std::string &path);
void writeTextFileToDisk(const std::string &path, const std::string &content);

bool fileExists(const std::string &name);

bool filePathExists(const std::string &name);

bool isPathDirectory(const std::string &name);
} // namespace SirMetal

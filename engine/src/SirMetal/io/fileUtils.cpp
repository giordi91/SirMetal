#include "SirMetal/io/fileUtils.h"

#include <filesystem>
#include <fstream>

namespace SirMetal {

void listFilesInFolder(const char *folderPath,
                       std::vector<std::string> &filePaths,
                       const std::string &wantedExtension) {
  bool shouldFilter = !wantedExtension.empty();
  const std::string extension = "." + wantedExtension;
  auto program_p = std::filesystem::path(folderPath);
  auto dirIt = std::filesystem::directory_iterator(program_p);
  for (const auto &p : dirIt) {
    bool isDir = std::filesystem::is_directory(p);
    if (!isDir) {
      auto path = std::filesystem::path(p);

      if (shouldFilter && !(path.extension() == extension)) {
        continue;
      }
      filePaths.push_back(path.u8string());
    }
  }
}

std::string getFileName(const std::string &path) {
  const auto expPath = std::filesystem::path(path);
  return expPath.stem().string();
}

std::string getFileExtension(const std::string &path) {
  const auto expPath = std::filesystem::path(path);
  return expPath.extension().string();
}

std::string getPathName(const std::string &path) {
  const auto expPath = std::filesystem::path(path);
  return expPath.parent_path().string();
}

void writeTextFileToDisk(const std::string &path, const std::string &content) {
  std::ofstream myfile;
  myfile.open(path, std::ostream::out | std::ostream::trunc);
  myfile << content;
  myfile.close();
}

bool fileExists(const std::string &name) {
  return std::filesystem::exists(name);
}

bool filePathExists(const std::string &name) {
  const std::filesystem::path path(name);
  const std::filesystem::path parent = path.parent_path();
  return std::filesystem::exists(parent);
}

bool isPathDirectory(const std::string &name) {
  return std::filesystem::is_directory(name);
}

} // namespace SirMetal

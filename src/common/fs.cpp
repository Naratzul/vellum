#include "fs.h"

#include <filesystem>
#include <fstream>

namespace vellum {
namespace common {

namespace fs = std::filesystem;

std::string readFileContent(std::string_view path) {
  fs::path fsPath(path);
  if (!fs::exists(fsPath)) {
    throw std::runtime_error("File doesn't exist: " + fsPath.string());
  }

  std::ifstream file(fsPath);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + fsPath.string());
  }

  return std::string{std::istreambuf_iterator<char>(file),
                     std::istreambuf_iterator<char>()};
}

void writeBinaryToFile(std::string_view path, std::string_view binaryContent)
{
  fs::path fsPath(path);
  std::ofstream file(fsPath, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + fsPath.string());
  }

  file.write(binaryContent.data(), binaryContent.length());
}

std::string canonicalPath(std::string_view path) {
  return fs::canonical(fs::path(path)).string();
}

std::string filenameWithoutExt(std::string_view path) {
  return fs::path(path).stem().string();
}

std::string replaceFilename(std::string_view path,
                            std::string_view newFileName) {
  return (fs::path(path).parent_path() / newFileName).string();
}

std::string replaceExtension(std::string_view path, std::string_view newExt) {
  return fs::path(path).replace_extension(newExt).string();
}

}  // namespace common
}  // namespace vellum
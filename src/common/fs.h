#pragma once

#include <filesystem>
#include <string>

#include "types.h"

namespace fs = std::filesystem;

namespace vellum {
namespace common {
std::string readFileContent(const fs::path& path);
void writeBinaryToFile(std::string_view path, std::string_view content);

std::string canonicalPath(std::string_view path);
std::string filenameWithoutExt(std::string_view path);
std::string replaceFilename(std::string_view path,
                            std::string_view newFileName);
std::string replaceExtension(std::string_view path, std::string_view newExt);

Vec<std::string> dedupePathsPreserveOrder(
    const Vec<std::string>& paths, const fs::path& base = fs::current_path());
}  // namespace common
}  // namespace vellum
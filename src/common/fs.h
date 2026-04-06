#pragma once

#include <filesystem>
#include <string>

#include "types.h"

namespace vellum {
namespace common {

namespace fs = std::filesystem;

std::string readFileContent(const fs::path& path);
void writeBinaryToFile(const fs::path& path, std::string_view content);

std::string filenameWithoutExt(std::string_view path);

Vec<fs::path> dedupePathsPreserveOrder(const Vec<fs::path>& paths);
}  // namespace common
}  // namespace vellum
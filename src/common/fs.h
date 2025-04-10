#pragma once

#include <string>

namespace vellum {
namespace common {
std::string readFileContent(std::string_view path);
void writeBinaryToFile(std::string_view path, std::string_view content);

std::string canonicalPath(std::string_view path);
std::string filenameWithoutEx(std::string_view path);
std::string replaceFilename(std::string_view path,
                            std::string_view newFileName);
std::string replaceExtension(std::string_view path, std::string_view newExt);
}  // namespace common
}  // namespace vellum
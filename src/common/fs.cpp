#include "fs.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

#include "os.h"

namespace vellum {
namespace common {

namespace {
fs::path makePathKey(const fs::path& path) {
  std::error_code ec;
  fs::path p = fs::weakly_canonical(path, ec);
  if (ec) {
    p = fs::absolute(path).lexically_normal();
  } else {
    p = p.lexically_normal();
  }
  return p;
}
}  // namespace

std::string readFileContent(const fs::path& path) {
  if (!fs::exists(path)) {
    throw std::runtime_error("File doesn't exist: " + pathToUtf8(path));
  }

  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + pathToUtf8(path));
  }

  return std::string{std::istreambuf_iterator<char>(file),
                     std::istreambuf_iterator<char>()};
}

void writeBinaryToFile(const fs::path& path, std::string_view binaryContent) {
  std::ofstream file(path, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + pathToUtf8(path));
  }

  file.write(binaryContent.data(), binaryContent.length());
}

std::string filenameWithoutExt(std::string_view path) {
  return fs::path(path).stem().string();
}

Vec<fs::path> dedupePathsPreserveOrder(const Vec<fs::path>& paths) {
  Vec<fs::path> out;
  out.reserve(paths.size());
  Set<fs::path> seen;
  seen.reserve(paths.size() * 2);
  for (const auto& s : paths) {
    auto key = makePathKey(s);
    if (seen.insert(std::move(key)).second) out.push_back(s);
  }
  return out;
}

}  // namespace common
}  // namespace vellum
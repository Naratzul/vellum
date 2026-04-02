#include "fs.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace vellum {
namespace common {

namespace {
std::string makePathKey(const std::string& pathStr,
                        const fs::path& base) {
  fs::path raw(pathStr);
  std::error_code ec;
  fs::path p = fs::weakly_canonical(raw, ec);
  if (ec) {
    p = fs::absolute(raw).lexically_normal();
  } else {
    p = p.lexically_normal();
  }
  std::string k = p.string();

#ifdef _WIN32
  std::transform(k.begin(), k.end(), k.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
#endif

  return k;
}
}  // namespace

std::string readFileContent(const fs::path& path) {
  if (!fs::exists(path)) {
    throw std::runtime_error("File doesn't exist: " + path.string());
  }

  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + path.string());
  }

  return std::string{std::istreambuf_iterator<char>(file),
                     std::istreambuf_iterator<char>()};
}

void writeBinaryToFile(std::string_view path, std::string_view binaryContent) {
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

Vec<std::string> dedupePathsPreserveOrder(const Vec<std::string>& paths,
                                          const fs::path& base) {
  Vec<std::string> out;
  out.reserve(paths.size());
  Set<std::string> seen;
  seen.reserve(paths.size() * 2);
  for (const auto& s : paths) {
    std::string key = makePathKey(s, base);
    if (seen.insert(std::move(key)).second) out.push_back(s);
  }
  return out;
}

}  // namespace common
}  // namespace vellum
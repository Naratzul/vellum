#pragma once

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace vellum {
class Vellum {
 public:
  bool run(const fs::path& inputFile,
           const std::vector<fs::path>& importPaths,
           bool emitDebugInfo = true);
};
}  // namespace vellum
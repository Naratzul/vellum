#pragma once

#include <filesystem>
#include <vector>

#include "common/types.h"

namespace fs = std::filesystem;

namespace vellum {
class Vellum {
 public:
  bool run(const fs::path& inputFile,
           const std::vector<fs::path>& importPaths,
           bool emitDebugInfo = true,
           common::Opt<fs::path> outputDirectory = std::nullopt);
};
}  // namespace vellum
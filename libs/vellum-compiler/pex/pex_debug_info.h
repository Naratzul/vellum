#pragma once

#include <ctime>
#include <memory>
#include <vector>

#include "common/types.h"
#include "pex_debug_function_info.h"

namespace vellum {
namespace pex {
using common::Vec;

struct PexDebugInfo {
  std::time_t modificationTime = 0;
  Vec<PexDebugFunctionInfo> functions;
};

class PexWriter;
void write(const PexDebugInfo& info, PexWriter& writer);

}  // namespace pex
}  // namespace vellum

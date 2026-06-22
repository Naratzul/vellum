#pragma once

#include <vector>

#include "common/types.h"
#include "pex_string.h"

namespace vellum {
namespace pex {
using common::Vec;

enum class PexDebugFunctionType : uint8_t {
  Normal = 0,
  Getter = 1,
  Setter = 2,
};

struct PexDebugFunctionInfo {
  PexString objectName;
  PexString stateName;
  PexString functionName;
  PexDebugFunctionType functionType = PexDebugFunctionType::Normal;
  Vec<uint16_t> instructionLineMap;
};

class PexWriter;
void write(const PexDebugFunctionInfo& info, PexWriter& writer);

}  // namespace pex
}  // namespace vellum

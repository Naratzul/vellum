#include "pex_debug_function_info.h"

#include "pex_writer.h"

namespace vellum {
namespace pex {

void write(const PexDebugFunctionInfo& info, PexWriter& writer) {
  writer << info.objectName << info.stateName << info.functionName
         << static_cast<uint8_t>(info.functionType)
         << info.instructionLineMap;
}

}  // namespace pex
}  // namespace vellum

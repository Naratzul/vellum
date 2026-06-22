#include "pex_debug_info.h"

#include "pex_debug_function_info.h"
#include "pex_writer.h"

namespace vellum {
namespace pex {

void write(const PexDebugInfo& info, PexWriter& writer) {
  writer << static_cast<int64_t>(info.modificationTime);
  writer << static_cast<uint16_t>(info.functions.size());
  for (const auto& f : info.functions) {
    write(f, writer);
  }
}

}  // namespace pex
}  // namespace vellum

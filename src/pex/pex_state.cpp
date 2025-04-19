#include "pex_state.h"

#include "pex_writer.h"

namespace vellum {
namespace pex {
PexWriter& operator<<(PexWriter& writer, const PexState& state) {
  writer << state.getName() << state.getFunctions();
  return writer;
}
}  // namespace pex
}  // namespace vellum
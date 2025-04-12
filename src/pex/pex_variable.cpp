#include "pex_variable.h"

#include "pex_writer.h"

namespace vellum {
namespace pex {
PexWriter& operator<<(PexWriter& writer, const PexVariable& var) {
  writer << var.name() << var.typeName() << var.userFlags()
         << var.defaultValue();
  return writer;
}
}  // namespace pex
}  // namespace vellum
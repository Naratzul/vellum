#include "pex_variable.h"

#include "pex_writer.h"

namespace vellum {
namespace pex {
PexWriter& operator<<(PexWriter& writer, const PexVariable& var) {
  writer << var.name() << var.typeName() << var.userFlags()
         << var.defaultValue();
  return writer;
}

bool operator==(const PexVariable& lhs, const PexVariable& rhs) {
  return lhs.name() == rhs.name() && lhs.typeName() == rhs.typeName() &&
         lhs.userFlags() == rhs.userFlags() &&
         lhs.defaultValue() == rhs.defaultValue();
}

bool operator!=(const PexVariable& lhs, const PexVariable& rhs) {
  return false;
}
}  // namespace pex
}  // namespace vellum
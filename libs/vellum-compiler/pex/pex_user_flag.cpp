#include "pex_user_flag.h"

#include "pex_writer.h"

namespace vellum {
namespace pex {

PexWriter& operator<<(PexWriter& writer, const PexUserFlags& flags) {
  writer << flags.getValue();
  return writer;
}

bool operator==(const PexUserFlags& lhs, const PexUserFlags& rhs) {
  return lhs.getValue() == rhs.getValue();
}

bool operator!=(const PexUserFlags& lhs, const PexUserFlags& rhs) {
  return !(lhs == rhs);
}
}  // namespace pex
}  // namespace vellum
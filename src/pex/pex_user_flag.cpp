#include "pex_user_flag.h"

namespace vellum {
namespace pex {

PexWriter& operator<<(PexWriter& writer, const UserFlags& flags) {
  writer << (uint16_t)0;
  return writer;
}
}  // namespace pex
}  // namespace vellum
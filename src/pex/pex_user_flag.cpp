#include "pex_user_flag.h"

namespace vellum {
namespace pex {

PexWriter& operator<<(PexWriter& writer, const UserFlags& flags) {
  writer << 0x00;
  return writer;
}
}  // namespace pex
}  // namespace vellum
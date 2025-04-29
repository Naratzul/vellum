#include "pex_user_flag.h"

#include "pex_writer.h"

namespace vellum {
namespace pex {

PexWriter& operator<<(PexWriter& writer, const PexUserFlags& flags) {
  writer << flags.getValue();
  return writer;
}
}  // namespace pex
}  // namespace vellum
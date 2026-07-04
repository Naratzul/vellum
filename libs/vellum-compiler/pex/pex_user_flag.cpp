#include "pex_user_flag.h"

#include "pex_writer.h"

namespace vellum {
namespace pex {

PexWriter& operator<<(PexWriter& writer, const PexUserFlags& flags) {
  writer << (uint32_t)flags;
  return writer;
}

std::string_view userFlagToString(PexUserFlag flag) {
  switch (flag) {
    case PexUserFlag::Hidden:
      return "hidden";
    case PexUserFlag::Conditional:
      return "conditional";
  }

  assert(false && "Unknown PexUserFlag");

  return "unknown";
}

uint8_t userFlagBitIndex(PexUserFlag flag) {
  switch (flag) {
    case PexUserFlag::Hidden:
      return 0;
    case PexUserFlag::Conditional:
      return 1;
  }

  assert(false && "Unknown PexUserFlag");

  return -1;
}

}  // namespace pex
}  // namespace vellum
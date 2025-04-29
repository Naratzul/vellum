#pragma once

#include <cstdint>

namespace vellum {
namespace pex {
class PexWriter;

class PexUserFlags {
 public:
  PexUserFlags() = default;
  PexUserFlags(uint32_t value) : value(value) {}
  uint32_t getValue() const { return value; }

 private:
  uint32_t value = 0;
};

PexWriter& operator<<(PexWriter& writer, const PexUserFlags& flags);

}  // namespace pex
}  // namespace vellum
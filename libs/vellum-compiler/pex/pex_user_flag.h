#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "common/flags.h"

namespace vellum {
namespace pex {
class PexWriter;

enum class PexUserFlag : uint32_t {
  None = 0,
  Hidden = 1 << 0,
  Conditional = 1 << 1
};

inline constexpr auto allUserFlags =
    std::array{PexUserFlag::Hidden, PexUserFlag::Conditional};

using PexUserFlags = common::Flags<PexUserFlag>;

PexWriter& operator<<(PexWriter& writer, const PexUserFlags& flags);

std::string_view userFlagToString(PexUserFlag flag);
uint8_t userFlagBitIndex(PexUserFlag flag);

}  // namespace pex
}  // namespace vellum
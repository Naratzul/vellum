#pragma once

#include <type_traits>

namespace vellum::common {
template <typename BitsType>
class Flags {
 public:
  using MaskType = typename std::underlying_type<BitsType>::type;

  constexpr Flags() = default;
  constexpr Flags(BitsType bits) : mask(static_cast<MaskType>(bits)) {}
  constexpr Flags(MaskType mask) : mask(mask) {}

  constexpr Flags<BitsType> operator|(
      const Flags<BitsType>& rhs) const noexcept {
    return Flags<BitsType>(mask | rhs.mask);
  }

  constexpr Flags<BitsType>& operator|=(
      const Flags<BitsType>& rhs) noexcept {
    mask |= rhs.mask;
    return *this;
  }

  explicit constexpr operator MaskType() const noexcept { return mask; }

 private:
  MaskType mask{0};
};
}  // namespace vellum::common
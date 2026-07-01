#pragma once

#include <type_traits>
#include <compare>

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

  constexpr Flags<BitsType>& operator|=(const Flags<BitsType>& rhs) noexcept {
    mask |= rhs.mask;
    return *this;
  }

  constexpr Flags<BitsType> operator&(
      const Flags<BitsType>& rhs) const noexcept {
    return Flags(mask & rhs.mask);
  }

  constexpr Flags<BitsType>& operator&=(const Flags<BitsType>& rhs) noexcept {
    mask &= rhs.mask;
    return *this;
  }

  explicit constexpr operator MaskType() const noexcept { return mask; }

  constexpr operator bool() const noexcept { return !!mask; }

  constexpr auto operator<=>(const Flags<BitsType>& rhs) const noexcept = default;

 private:
  MaskType mask{0};
};
}  // namespace vellum::common
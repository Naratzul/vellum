#pragma once

#include <type_traits>

#include "common/flags.h"
#include "common/types.h"
#include "lexer/token.h"

namespace vellum {
using common::Opt;
using common::Vec;

enum class VellumModifier {
  None = 0,
  Auto = 1 << 0,
  Conditional = 1 << 1,
  Hidden = 1 << 2,
  Native = 1 << 3,
  Static = 1 << 4
};

using VellumModifiers = common::Flags<VellumModifier>;

enum class VellumModifierContext {
  Script,
  State,
  Var,
  Property,
  Function,
  Event,
  Import
};

inline constexpr int operator|(VellumModifier lhs, VellumModifier rhs) {
  using UnderlyingT = std::underlying_type_t<VellumModifier>;
  return static_cast<UnderlyingT>(lhs) | static_cast<UnderlyingT>(rhs);
}

inline constexpr int operator&(VellumModifier lhs, VellumModifier rhs) {
  using UnderlyingT = std::underlying_type_t<VellumModifier>;
  return static_cast<UnderlyingT>(lhs) & static_cast<UnderlyingT>(rhs);
}

struct ParsedModifier {
  VellumModifier modifier;
  Token location;
};

using ParsedModifiers = Vec<ParsedModifier>;

VellumModifiers allowedModifiersFor(VellumModifierContext context);
const char* modifierName(VellumModifier modifier);
VellumModifiers modifiersBitmask(const ParsedModifiers& modifiers);
bool hasModifier(const ParsedModifiers& modifiers, VellumModifier modifier);
Opt<Token> findModifierLocation(const ParsedModifiers& modifiers,
                                VellumModifier modifier);

}  // namespace vellum
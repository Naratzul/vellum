#include "vellum_modifier.h"

namespace vellum {
VellumModifiers allowedModifiersFor(VellumModifierContext context) {
  using enum VellumModifierContext;
  using enum VellumModifier;

  switch (context) {
    case Script:
      return Hidden | Conditional;
    case State:
      return Auto;
    case Var:
      return Conditional;
    case Property:
      return Hidden;
    case Function:
      return Static | Native;
    case Event:
      return None;
    case Import:
      return None;
  }
}

const char* modifierName(VellumModifier modifier) {
  switch (modifier) {
    case VellumModifier::Auto:
      return "auto";
    case VellumModifier::Conditional:
      return "conditional";
    case VellumModifier::Hidden:
      return "hidden";
    case VellumModifier::Native:
      return "native";
    case VellumModifier::Static:
      return "static";
    default:
      return "unknown";
  }
}

VellumModifiers modifiersBitmask(const ParsedModifiers& modifiers) {
  VellumModifiers result{VellumModifier::None};
  for (const ParsedModifier& parsed : modifiers) {
    result |= parsed.modifier;
  }
  return result;
}

bool hasModifier(const ParsedModifiers& modifiers, VellumModifier modifier) {
  for (const ParsedModifier& parsed : modifiers) {
    if (parsed.modifier == modifier) {
      return true;
    }
  }
  return false;
}

Opt<Token> findModifierLocation(const ParsedModifiers& modifiers,
                                VellumModifier modifier) {
  for (const ParsedModifier& parsed : modifiers) {
    if (parsed.modifier == modifier) {
      return parsed.location;
    }
  }
  return std::nullopt;
}
}  // namespace vellum
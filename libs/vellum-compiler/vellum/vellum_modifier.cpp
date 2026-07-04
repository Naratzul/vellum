#include "vellum_modifier.h"

#include "compiler/compiler_error_handler.h"

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
      return Hidden | Conditional;
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

bool modifiersAreValid(const ParsedModifiers& modifiers,
                       VellumModifierContext context) {
  const VellumModifiers allowed = allowedModifiersFor(context);
  for (const ParsedModifier& parsed : modifiers) {
    if (!(allowed & parsed.modifier)) {
      return false;
    }
  }
  return true;
}

void validateModifiersContext(const ParsedModifiers& modifiers,
                              VellumModifierContext context,
                              CompilerErrorHandler& errorHandler) {
  const VellumModifiers allowed = allowedModifiersFor(context);
  for (const ParsedModifier& parsed : modifiers) {
    if (!(allowed & parsed.modifier)) {
      errorHandler.errorAt(parsed.location, CompilerErrorKind::InvalidModifier,
                           "Modifier '{}' is not valid here.",
                           modifierName(parsed.modifier));
    }
  }
}

pex::PexUserFlags buildPexFlags(const ParsedModifiers& modifiers) {
  pex::PexUserFlags flags{pex::PexUserFlag::None};

  for (const auto& modifier : modifiers) {
    switch (modifier.modifier) {
      case VellumModifier::Conditional:
        flags |= pex::PexUserFlag::Conditional;
        break;
      case VellumModifier::Hidden:
        flags |= pex::PexUserFlag::Hidden;
        break;
      default:
        break;
    }
  }

  return flags;
}

pex::PexUserFlags buildPropertyPexFlags(const ParsedModifiers& modifiers) {
  pex::PexUserFlags flags{pex::PexUserFlag::None};

  for (const auto& modifier : modifiers) {
    switch (modifier.modifier) {
      case VellumModifier::Hidden:
        flags |= pex::PexUserFlag::Hidden;
        break;
      default:
        break;
    }
  }

  return flags;
}

pex::PexUserFlags buildVariablePexFlags(const ParsedModifiers& modifiers) {
  pex::PexUserFlags flags{pex::PexUserFlag::None};

  for (const auto& modifier : modifiers) {
    switch (modifier.modifier) {
      case VellumModifier::Conditional:
        flags |= pex::PexUserFlag::Conditional;
        break;
      default:
        break;
    }
  }

  return flags;
}
}  // namespace vellum
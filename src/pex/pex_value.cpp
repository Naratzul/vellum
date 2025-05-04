#include "pex_value.h"

namespace vellum {
namespace pex {

bool operator==(const PexValue& lhs, const PexValue& rhs) {
  if (lhs.getType() != rhs.getType()) {
    return false;
  }

  switch (lhs.getType()) {
    case PexValueType::Bool:
      return lhs.asBool() == rhs.asBool();
    case PexValueType::Float:
      return lhs.asFloat() == rhs.asFloat();
    case PexValueType::Identifier:
      return lhs.asIdentifier() == rhs.asIdentifier();
    case PexValueType::Integer:
      return lhs.asInt() == rhs.asInt();
    case PexValueType::String:
      return lhs.asString() == rhs.asString();
    case PexValueType::None:
      return true;
    default:
      assert(false && "Unsupported pex value type.");
  }
}

bool operator!=(const PexValue& lhs, const PexValue& rhs) {
  return !(lhs == rhs);
}
}  // namespace pex
}  // namespace vellum

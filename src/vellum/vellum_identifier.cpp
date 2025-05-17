#include "vellum_identifier.h"

namespace vellum {
bool operator==(const VellumIdentifier& lhs, const VellumIdentifier& rhs) {
  return lhs.getValue() == rhs.getValue();
}

bool operator!=(const VellumIdentifier& lhs, const VellumIdentifier& rhs) {
  return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const VellumIdentifier& id) {
  os << id.getValue();
  return os;
}
}  // namespace vellum
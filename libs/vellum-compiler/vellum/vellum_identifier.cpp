#include "vellum_identifier.h"

#include <algorithm>
#include <cctype>

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

bool equalsCaseInsensitive(const VellumIdentifier& lhs, const VellumIdentifier& rhs) {
  auto lhsValue = lhs.getValue();
  auto rhsValue = rhs.getValue();
  
  if (lhsValue.size() != rhsValue.size()) {
    return false;
  }
  
  return std::equal(lhsValue.begin(), lhsValue.end(), rhsValue.begin(),
                    [](char a, char b) {
                      return std::tolower(static_cast<unsigned char>(a)) ==
                             std::tolower(static_cast<unsigned char>(b));
                    });
}
}  // namespace vellum
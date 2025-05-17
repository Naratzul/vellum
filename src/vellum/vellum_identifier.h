#pragma once

#include <iostream>
#include <string_view>

namespace vellum {

class VellumIdentifier {
 public:
  explicit VellumIdentifier(std::string_view value) : value(value) {}
  std::string_view getValue() const { return value; }
  std::string_view toString() const { return value; }

 private:
  std::string_view value;
};

bool operator==(const VellumIdentifier& lhs, const VellumIdentifier& rhs);
bool operator!=(const VellumIdentifier& lhs, const VellumIdentifier& rhs);
std::ostream& operator<<(std::ostream& os, const VellumIdentifier& id);
}  // namespace vellum
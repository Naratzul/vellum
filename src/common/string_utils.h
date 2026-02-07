#pragma once

#include <cctype>
#include <string>
#include <string_view>

#include "common/string_set.h"

namespace vellum {
namespace common {

inline std::string_view normalizeToLower(std::string_view identifier) {
  std::string result;
  result.reserve(identifier.size());
  for (char c : identifier) {
    result += std::tolower(static_cast<unsigned char>(c));
  }
  return StringSet::insert(result);
}

}  // namespace common
}  // namespace vellum

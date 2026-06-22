#pragma once

#include <algorithm>
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

inline void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

inline void trim(std::string &s) {
  rtrim(s);
  ltrim(s);
}

}  // namespace common
}  // namespace vellum

#pragma once

#include <set>
#include <string>
#include <string_view>

namespace vellum {
namespace common {
class StringSet {
 public:
  static std::string_view insert(std::string_view str);

 private:
  std::set<std::string> set;

  static StringSet& instance();

  StringSet() = default;
};
}  // namespace common
}  // namespace vellum
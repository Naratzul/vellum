#include "string_set.h"

namespace vellum {
namespace common {
std::string_view StringSet::insert(std::string_view str) {
  return std::string_view(*instance().set.insert(std::string(str)).first);
}

StringSet& StringSet::instance() {
  static StringSet instance;
  return instance;
}
}  // namespace common
}  // namespace vellum
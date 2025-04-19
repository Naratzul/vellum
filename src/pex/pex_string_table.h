#pragma once

#include <string_view>
#include <vector>

#include "pex_string.h"

namespace vellum {
namespace pex {

class PexWriter;

class PexStringTable {
 public:
  PexString lookup(std::string_view str);

  std::string_view valueByIndex(std::size_t index) const {
    return strings[index].str;
  }
  std::size_t size() const { return strings.size(); }

 private:
  struct Entry {
    std::size_t index;
    std::string_view str;
  };

  std::vector<Entry> strings;
};

PexWriter& operator<<(PexWriter& writer, const PexStringTable& table);
}  // namespace pex
}  // namespace vellum
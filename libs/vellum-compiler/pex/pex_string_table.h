#pragma once

#include <string_view>
#include <vector>

#include "common/types.h"
#include "pex_string.h"

namespace vellum {
namespace pex {
using common::Vec;

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

  Vec<Entry> strings;
};

PexWriter& operator<<(PexWriter& writer, const PexStringTable& table);
}  // namespace pex
}  // namespace vellum
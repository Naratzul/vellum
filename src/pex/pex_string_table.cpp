#include "pex_string_table.h"

#include <algorithm>

namespace vellum {
namespace pex {
PexString PexStringTable::lookup(std::string_view str) {
  auto it = std::ranges::find_if(
      strings, [str](const Entry& entry) { return entry.str == str; });
  if (it == strings.end()) {
    strings.push_back(Entry{strings.size(), str});
    it = std::prev(strings.end());
  }
  return PexString(it->index);
}

PexWriter& operator<<(PexWriter& writer, const PexStringTable& table) {
  writer << (uint16_t)table.size();
  for (int i = 0; i < table.size(); ++i) {
    writer << table.valueByIndex(i);
  }
  return writer;
}

}  // namespace pex
}  // namespace vellum
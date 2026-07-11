#pragma once

#include <format>
#include <string_view>

#include "common/string_set.h"
#include "vellum/vellum_identifier.h"

namespace vellum {

inline VellumIdentifier mangleLocalPexName(std::string_view name,
                                           size_t suffix) {
  auto str = std::format("{}_{}", name, suffix);
  return VellumIdentifier(common::StringSet::insert(str));
}

}  // namespace vellum

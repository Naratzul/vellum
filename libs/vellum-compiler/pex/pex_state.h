#pragma once

#include <vector>

#include "common/types.h"
#include "pex_function.h"
#include "pex_string.h"

namespace vellum {
namespace pex {
using common::Vec;

class PexWriter;

class PexState {
 public:
  PexState(PexString name, Vec<PexFunction> functions = {})
      : name(name), functions(std::move(functions)) {}

  PexString getName() const { return name; }
  const Vec<PexFunction>& getFunctions() const { return functions; }
  Vec<PexFunction>& getFunctions() { return functions; }

 private:
  PexString name;
  Vec<PexFunction> functions;
};

PexWriter& operator<<(PexWriter& writer, const PexState& state);
}  // namespace pex
}  // namespace vellum
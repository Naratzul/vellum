#pragma once

#include <vector>

#include "pex_function.h"
#include "pex_string.h"

namespace vellum {
namespace pex {

class PexWriter;

class PexState {
 public:
  PexState(PexString name, std::vector<PexFunction> functions = {})
      : name(name), functions(std::move(functions)) {}

  PexString getName() const { return name; }
  const std::vector<PexFunction>& getFunctions() const { return functions; }
  std::vector<PexFunction>& getFunctions() { return functions; }

 private:
  PexString name;
  std::vector<PexFunction> functions;
};

PexWriter& operator<<(PexWriter& writer, const PexState& state);
}  // namespace pex
}  // namespace vellum
#pragma once

#include "pex_string.h"

namespace vellum {
namespace pex {

class PexWriter;

class PexFunctionParameter {
 public:
  PexFunctionParameter(PexString name, PexString type)
      : name(name), type(type) {}

  PexString getName() const { return name; }
  PexString getType() const { return type; }

 private:
  PexString name;
  PexString type;
};

PexWriter& operator<<(PexWriter& writer, const PexFunctionParameter& param);
}  // namespace pex
}  // namespace vellum
#pragma once

#include "pex_string.h"

namespace vellum {
namespace pex {

class PexIdentifier {
 public:
  explicit PexIdentifier(PexString value) : value(value) {}
  PexString getValue() const { return value; }

 private:
  PexString value;
};

}  // namespace pex
}  // namespace vellum
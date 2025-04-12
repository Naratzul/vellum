#pragma once

#include "pex_value.h"

namespace vellum {
namespace pex {

class PexWriter;

class PexVariable final {
 public:
  PexVariable(PexString name, PexString typeName, PexValue defaultValue)
      : name_(name), typeName_(typeName), defaultValue_(defaultValue) {}

  PexString name() const { return name_; }
  PexString typeName() const { return typeName_; }
  PexValue defaultValue() const { return defaultValue_; }
  uint32_t userFlags() const { return userFlags_; }

 private:
  PexString name_;
  PexString typeName_;
  PexValue defaultValue_;
  uint32_t userFlags_ = 0;
};

PexWriter& operator<<(PexWriter& writer, const PexVariable& var);
}  // namespace pex
}  // namespace vellum
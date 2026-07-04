#pragma once

#include "pex_user_flag.h"
#include "pex_value.h"

namespace vellum {
namespace pex {

class PexWriter;

class PexVariable final {
 public:
  PexVariable(PexString name, PexString typeName, PexUserFlags userFlags,
              PexValue defaultValue)
      : name_(name),
        typeName_(typeName),
        userFlags_(userFlags),
        defaultValue_(defaultValue) {}

  PexString name() const { return name_; }
  PexString typeName() const { return typeName_; }
  PexUserFlags userFlags() const { return userFlags_; }
  PexValue defaultValue() const { return defaultValue_; }

 private:
  PexString name_;
  PexString typeName_;
  PexUserFlags userFlags_;
  PexValue defaultValue_;
};

PexWriter& operator<<(PexWriter& writer, const PexVariable& var);
bool operator==(const PexVariable& lhs, const PexVariable& rhs);
bool operator!=(const PexVariable& lhs, const PexVariable& rhs);
}  // namespace pex
}  // namespace vellum
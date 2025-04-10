#pragma once

#include "pex_string.h"
#include "pex_writer.h"

namespace vellum {
namespace pex {
class PexObject {
 public:
  PexString getName() const { return name; }
  void setName(PexString name_) { name = name_; }

  PexString getParentName() const { return parentName; }
  void setParentName(PexString parentName_) { parentName = parentName_; }

 private:
  PexString name;
  PexString parentName;
};

PexWriter& operator<<(PexWriter& writer, const PexObject& object);

}  // namespace pex
}  // namespace vellum